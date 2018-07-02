#include <sys/stat.h>
#include <err.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include <CoreServices/CoreServices.h>

#define VERSION "1.0.1-beta"

FSEventStreamRef stream;

struct path_map {
	char	*resolved;
	int	 resolvedlen;
	char	*orig;
	int	 origlen;
};

void graceful_shutdown(int);
void cb(ConstFSEventStreamRef, void *, size_t numEvents, void *,
    const FSEventStreamEventFlags *, const FSEventStreamEventId *);

static struct path_map **path_maps;
static int debug;

/* options */
static int preflight, subdir;

static int print_usage(FILE *fp);

int
main(int argc, char *argv[])
{
	CFStringRef *tmp_path, *pp;
	struct path_map **pm;
	struct stat st;
	char resolved[PATH_MAX + 1], c;
	int i, j;

	while ((c = getopt(argc, argv, "dhpsV")) != -1) {
		switch (c) {
		case 'd':
			debug = 1;
			break;
		case 'h':
			print_usage(stdout);
			exit(0);
		case 'p':
			preflight = 1;
			break;
		case 's':
			subdir = 1;
			break;
		case 'V':
			printf("version " VERSION "\n");
			exit(0);
		case '?':
			exit(1);
		}
	}

	argc -= optind;
	argv += optind;

	if (argc < 1) {
		print_usage(stderr);
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

	if ((path_maps = calloc(argc + 1, sizeof(struct path_map **))) == NULL)
		err(1, "calloc path_maps");
	path_maps[argc] = NULL;

	pm = path_maps;

	pp = tmp_path;
	if (debug)
		fprintf(stderr, "watching");
	for (i = 0; i < argc; i++) {
		if (realpath(argv[i], resolved) == NULL)
			err(1, "realpath");

		if ((*pm = malloc(sizeof(struct path_map))) == NULL)
			err(1, "malloc path_map");

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
		if (debug)
			fprintf(stderr, " %s", resolved);
	}

	/* signal path map end */
	*pm = NULL;

	if (debug)
		fprintf(stderr, "\n");

	stream = FSEventStreamCreate(NULL,
		&cb,
		NULL, /* callback info */
		CFArrayCreate(NULL, (const void **)tmp_path, argc, NULL),
		kFSEventStreamEventIdSinceNow, /* only changes from now on */
		0.03, /* latency in seconds */
		kFSEventStreamCreateFlagNone
	);

	if (signal(SIGINT, graceful_shutdown) == SIG_ERR)
		err(1, "signal");
	if (signal(SIGTERM, graceful_shutdown) == SIG_ERR)
		err(1, "signal");

	FSEventStreamScheduleWithRunLoop(stream, CFRunLoopGetCurrent(),
		kCFRunLoopDefaultMode);

	if (preflight)
		for (pm = path_maps; *pm; pm++) {
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

void cb(ConstFSEventStreamRef stream_ref,
    void *client_cbinfo,
    size_t  num_events,
    void *event_paths,
    const FSEventStreamEventFlags event_flags[],
    const FSEventStreamEventId event_ids[])
{
	struct	path_map **pm;
	char	**paths;
	int	i, soff; /* string offset */

	paths = event_paths;

	for (i = 0; i < num_events; i++) {
		if (event_flags[i] != kFSEventStreamEventFlagNone)
			warnx("flags present: %x %s", event_flags[i], paths[i]);

		/* Translate to user supplied path, optionally with subdir. */
		for (pm = path_maps; *pm; pm++) {
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

void
graceful_shutdown(int sig)
{
	FSEventStreamStop(stream);
	FSEventStreamInvalidate(stream);
	FSEventStreamRelease(stream);

	exit(0);
}

int
print_usage(FILE *fp)
{
	return fprintf(fp, "usage: %s [-hpsV] dir ...\n", getprogname());
}
