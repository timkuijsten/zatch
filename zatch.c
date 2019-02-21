/*
 * Copyright (c) 2017, 2018, 2019 Tim Kuijsten
 *
 * Permission to use, copy, modify, and distribute this software for any purpose
 * with or without fee is hereby granted, provided that the above copyright
 * notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/stat.h>

#include <assert.h>
#include <err.h>
#include <signal.h>
#define _DARWIN_BETTER_REALPATH
#include <stdio.h>
#include <stdlib.h>

#include <CoreServices/CoreServices.h>

#define VERSION "1.1.0-rc"

struct pathmap {
	char	*resolved;	/* with terminating slash and terminating nul */
	size_t	 resolvedlen;	/* excluding the terminating nul */
	char	*orig;	/* with terminating slash and terminating nul */
};

static struct pathmap **pms;
static size_t pmssize;

/* options */
static int preflight, subdir, verbose;

static FSEventStreamRef stream;

static void
cb(ConstFSEventStreamRef stream_ref, void *cbinfo, size_t nevents,
    void *evpaths, const FSEventStreamEventFlags evflags[],
    const FSEventStreamEventId evids[])
{
	struct pathmap *pm;
	const char **paths;
	size_t i, j, off, left;
	int rc;

	paths = evpaths;

	for (i = 0; i < nevents; i++) {
		if (evflags[i] != kFSEventStreamEventFlagNone) {
			if (verbose > -1)
				warnx("flags present: %x %s", evflags[i],
				    paths[i]);
			continue;
		}

		/*
		 * Translate to user supplied path, optionally with subdir. Both
		 * paths from FSEvents as well as our own paths are guaranteed
		 * to have a trailing slash.
		 */

		off = 0;
		left = pmssize;
		while (left) {
			/* ceil of middle index */
			j = ((left + 1) / 2) - 1 + off;
			pm = pms[j];

			rc = strncmp(pm->resolved, paths[i], pm->resolvedlen);
			if (rc == 0) {
				if (subdir) {
					fputs(pm->orig, stdout);
					puts(&paths[i][pm->resolvedlen]);
				} else {
					puts(pm->orig);
				}
				break;
			} else if (rc > 0) { /* resolved > paths[i] */
				if (j > 0) {
					left = j - off;
				} else {
					break;
				}
			} else {
				if (j < pmssize) {
					off = j + 1;
					left = pmssize - off;
				} else {
					break;
				}
			}
		}
	}

	if (fflush(stdout) == EOF)
		err(1, "fflush");
}

static int
sortresolved(const void *a, const void *b)
{
	const struct pathmap **pm1, **pm2;

	pm1 = (const struct pathmap **)a;
	pm2 = (const struct pathmap **)b;

	return strcmp((*pm1)->resolved, (*pm2)->resolved);
}

static void
shutdown(int sig)
{
	CFRunLoopStop(CFRunLoopGetCurrent());
	FSEventStreamStop(stream);
	FSEventStreamInvalidate(stream);
	FSEventStreamRelease(stream);

	exit(0);
}

static int
printusage(FILE *fp)
{
	return fprintf(fp, "usage: %s [-Vpqsv] dir ...\n", getprogname());
}

int
main(int argc, char *argv[])
{
	CFStringRef *cfa;
	struct pathmap *pm;
	struct stat st;
	char rp[PATH_MAX + 1], c, *cp;
	size_t i, n, cfasize;

	while ((c = getopt(argc, argv, "Vhpqsv")) != -1) {
		switch (c) {
		case 'V':
			printf("version " VERSION "\n");
			exit(0);
		case 'h':
			printusage(stdout);
			exit(0);
		case 'p':
			preflight = 1;
			break;
		case 'q':
			verbose--;
			break;
		case 's':
			subdir = 1;
			break;
		case 'v':
			verbose++;
			break;
		case '?':
			exit(1);
		}
	}

	argc -= optind;
	argv += optind;

	if (argc < 1) {
		printusage(stderr);
		exit(1);
	}

	/*
	 * Create two arrays. One array is used on every occurrence of an event
	 * and maps a path name as given to the callback by the FSEvents API to
	 * the name used by the user as a parameter to this program. The other
	 * array is a list of all path names as a parameter for
	 * FSEventStreamCreate. Skip any given paths that are not a directory.
	 */

	pmssize = cfasize = 0;
	for (i = 0; i < (size_t)argc; i++) {
		if (stat(argv[i], &st) == -1) {
			if (verbose > -2)
				warn("%s", argv[i]);
			continue;
		}

		if (!S_ISDIR(st.st_mode)) {
			if (verbose > -1)
				warnx("%s: Not a directory", argv[i]);
			continue;
		}

		if (realpath(argv[i], rp) == NULL)
			err(1, "realpath");

		if ((pm = malloc(sizeof(*pm))) == NULL)
			err(1, "malloc pm");

		/*
		 * Copy the original dir as specified by the user but ensure a
		 * terminating slash and terminating nul.
		 */

		n = strlen(argv[i]) + 1;	/* include terminating nul */
		assert(n >= 2);
		if (argv[i][n - 2] != '/')
			n++;	/* add terminating slash */

		if ((cp = malloc(n)) == NULL)
			err(1, "malloc");

		memcpy(cp, argv[i], strlen(argv[i]));
		cp[n - 2] = '/';
		cp[n - 1] = '\0';
		pm->orig = cp;

		/*
		 * Same thing for the resolved path. Since the FSEvents API will
		 * return each path with a trailing slash this allows for easier
		 * matching in the callback. realpath(3) only yields a trailing
		 * slash on the root path "/" and always terminates the path
		 * with a nul.
		 */

		n = strlen(rp) + 1;	/* include terminating nul */
		assert(n >= 2);
		if (rp[n - 2] != '/')
			n++;	/* add terminating slash */

		if ((cp = malloc(n)) == NULL)
			err(1, "malloc");

		memcpy(cp, rp, strlen(rp));
		cp[n - 2] = '/';
		cp[n - 1] = '\0';
		pm->resolved = cp;
		pm->resolvedlen = n - 1;

		if (INT_MAX / sizeof(*pms) <= pmssize)
			errx(1, "overflow");
		if (INT_MAX / sizeof(*cfa) <= cfasize)
			errx(1, "overflow");
		if ((pms = realloc(pms, (pmssize + 1) * sizeof(*pms))) == NULL)
			err(1, "realloc pms");
		if ((cfa = realloc(cfa, (cfasize + 1) * sizeof(*cfa))) == NULL)
			err(1, "realloc cfa");
		pmssize++;
		cfasize++;

		pms[pmssize - 1] = pm;

		if ((cfa[cfasize - 1] = CFStringCreateWithCString(NULL, rp,
		    kCFStringEncodingUTF8)) == NULL)
			errx(1, "CFStringCreateWithCString");

		if (verbose > 0) {
			if (pmssize == 1)
				fprintf(stderr, "watching");

			fprintf(stderr, " %s", rp);
		}
	}

	if (pmssize == 0)
		errx(1, "no directories to watch");

	if (verbose > 0)
		fprintf(stderr, "\n");
	
	/* Make sure we can do a binary search on the resolved path. */
	qsort(pms, pmssize, sizeof(*pms), sortresolved);

	if (preflight) {
		for (i = 0; i < pmssize; i++)
			puts(pms[i]->orig);

		if (fflush(stdout) == EOF)
			err(1, "fflush");
	}

	stream = FSEventStreamCreate(NULL,
		&cb,
		NULL,
		CFArrayCreate(NULL, (const void **)cfa, cfasize, NULL),
		kFSEventStreamEventIdSinceNow,	/* only changes from now on */
		0.03,	/* latency in seconds */
		kFSEventStreamCreateFlagNone
	);

	if (signal(SIGINT, shutdown) == SIG_ERR)
		err(1, "signal");
	if (signal(SIGTERM, shutdown) == SIG_ERR)
		err(1, "signal");

	FSEventStreamScheduleWithRunLoop(stream, CFRunLoopGetCurrent(),
		kCFRunLoopDefaultMode);

	if (!FSEventStreamStart(stream))
		errx(1, "FSEventStreamStart");

	CFRunLoopRun();

	/* never reached */
	exit(2);
}
