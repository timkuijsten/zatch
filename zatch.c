#include <sys/stat.h>

#include <err.h>
#include <signal.h>
#define _DARWIN_BETTER_REALPATH
#include <stdio.h>
#include <stdlib.h>

#include <CoreServices/CoreServices.h>

#define VERSION "1.0.1-rc"

struct pathmap {
	char	*resolved;
	int	 resolvedlen;
	char	*orig;
	int	 origlen;
};

static struct pathmap **pathmaps;

/* options */
static int preflight, subdir, verbose;

static FSEventStreamRef stream;

static void
cb(ConstFSEventStreamRef stream_ref, void *cbinfo, size_t nevents,
    void *evpaths, const FSEventStreamEventFlags evflags[],
    const FSEventStreamEventId evids[])
{
	struct	pathmap **pm;
	char	**paths;
	int	i, soff; /* string offset */

	paths = evpaths;

	for (i = 0; i < nevents; i++) {
		if (evflags[i] != kFSEventStreamEventFlagNone)
			warnx("flags present: %x %s", evflags[i], paths[i]);

		/* Translate to user supplied path, optionally with subdir. */
		for (pm = pathmaps; *pm; pm++) {
			/*
			 * Resolved and paths are guaranteed to have a trailing
			 * slash.
			 */

			if (strncmp((*pm)->resolved, paths[i],
			    (*pm)->resolvedlen) != 0)
				continue;

			/* We have a match. */
			printf("%s", (*pm)->orig);

			/* Append subdir if requested. */
			if (subdir) {
				soff = (*pm)->resolvedlen;

				/*
				 * Only include the slash if the user did not
				 * supply one.
				 */
				if ((*pm)->orig[(*pm)->origlen - 1] != '/')
					soff--;
				printf("%s", &paths[i][soff]);
			}
			printf("\n");

			if (fflush(stdout) == EOF)
				err(1, "fflush");

			break;
		}
	}
}

static void
shutdown(int sig)
{
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
	CFStringRef *tmp_path, *pp;
	struct pathmap **pm;
	struct stat st;
	char resolved[PATH_MAX + 1], c;
	int i, j;

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

	/* remove all arguments that are not the name of a directory */
	for (i = 0; i < argc; i++) {
		if (stat(argv[i], &st) == -1 || !S_ISDIR(st.st_mode)) {
			warnx("%s: Not a directory", argv[i]);

			/* compact */
			for (j = i; j < argc; j++)
				argv[j] = argv[j + 1];
			i--;
			argc--;
			continue;
		}
	}

	if (argc < 1)
		errx(1, "no directories to watch");

	if ((tmp_path = calloc(argc, sizeof(CFStringRef))) == NULL)
		err(1, "calloc tmp_path");

	/*
	 * Create a NULL terminated list of pointers to path map structures.
	 */

	if ((pathmaps = calloc(argc + 1, sizeof(struct pathmap **))) == NULL)
		err(1, "calloc pathmaps");
	pathmaps[argc] = NULL;

	pm = pathmaps;

	pp = tmp_path;
	if (verbose > 0)
		fprintf(stderr, "watching");
	for (i = 0; i < argc; i++) {
		if (realpath(argv[i], resolved) == NULL)
			err(1, "realpath");

		if ((*pm = malloc(sizeof(struct pathmap))) == NULL)
			err(1, "malloc pathmap");

		(*pm)->orig = argv[i];
		(*pm)->origlen = strlen(argv[i]);
		(*pm)->resolvedlen = strlen(resolved);

		/*
		 * Ensure a trailing slash in the resolved path. Since the
		 * FSEvents API will return each path with a trailing slash this
		 * allows for easier matching in the callback.
		 */

		if (resolved[(*pm)->resolvedlen - 1] != '/') {
			(*pm)->resolvedlen++;
			if (asprintf(&(*pm)->resolved, "%s/", resolved) !=
			    (*pm)->resolvedlen)
				errx(1, "asprintf");
		} else {
			/* Only happens with the root. */
			if (((*pm)->resolved = strdup(resolved)) == NULL)
				err(1, "strdup");
		}
		pm++;

		if ((*pp++ = CFStringCreateWithCString(NULL, resolved,
		    kCFStringEncodingUTF8)) == NULL)
			errx(1, "CFStringCreateWithCString");
		if (verbose > 0)
			fprintf(stderr, " %s", resolved);
	}

	/* signal path map end */
	*pm = NULL;

	if (verbose > 0)
		fprintf(stderr, "\n");

	stream = FSEventStreamCreate(NULL,
		&cb,
		NULL, /* callback info */
		CFArrayCreate(NULL, (const void **)tmp_path, argc, NULL),
		kFSEventStreamEventIdSinceNow, /* only changes from now on */
		0.03, /* latency in seconds */
		kFSEventStreamCreateFlagNone
	);

	if (signal(SIGINT, shutdown) == SIG_ERR)
		err(1, "signal");
	if (signal(SIGTERM, shutdown) == SIG_ERR)
		err(1, "signal");

	FSEventStreamScheduleWithRunLoop(stream, CFRunLoopGetCurrent(),
		kCFRunLoopDefaultMode);

	if (preflight)
		for (pm = pathmaps; *pm; pm++) {
			/* ensure trailing slash */
			if (subdir && (*pm)->orig[(*pm)->origlen - 1] != '/')
				printf("%s/\n", (*pm)->orig);
			else
				printf("%s\n", (*pm)->orig);
			if (fflush(stdout) == EOF)
				err(1, "fflush");
		}

	/* start! */
	FSEventStreamStart(stream);
	CFRunLoopRun();

	/* should not reach */
	exit(2);
}
