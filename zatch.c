#include <sys/stat.h>
#include <err.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include <CoreServices/CoreServices.h>

extern char *__progname;

FSEventStreamRef stream;

void cb(ConstFSEventStreamRef, void *, size_t numEvents, void *, const FSEventStreamEventFlags *, const FSEventStreamEventId *);
void graceful_shutdown(int);

static int debug = 0;

int
main(int argc, char *argv[])
{
  int i;
  CFStringRef *tmp_paths, *pp;
  CFArrayRef paths;
  CFAbsoluteTime latency;
  char resolved[PATH_MAX + 1];
  struct stat st;

  if (argc < 2) {
    fprintf(stderr, "usage: %s <dir> ...\n", __progname);
    exit(1);
  }

  if ((tmp_paths = calloc(argc - 1, sizeof(CFStringRef))) == NULL)
    err(1, "malloc");

  /* make sure each parameter really is an existing directory */
  pp = tmp_paths;
  if (debug)
    fprintf(stderr, "watching");
  for (i = 1; i < argc; i++)
    if (stat(argv[i], &st) == -1) {
      err(1, "%s", argv[i]);
    } else {
      if (!S_ISDIR(st.st_mode))
        errx(1, "%s is not a directory", argv[i]);

      if (realpath(argv[i], resolved) == NULL)
        err(1, "realpath");

      if ((*pp++ = CFStringCreateWithCString(NULL, resolved, kCFStringEncodingUTF8)) == NULL)
        errx(1, "CFStringCreateWithCString");
      if (debug)
        fprintf(stderr, " %s", resolved);
    }

  if (debug)
    fprintf(stderr, "\n");

  paths = CFArrayCreate(NULL, (const void **)tmp_paths, argc - 1, NULL);

  latency = 0.01; /* latency in seconds */

  stream = FSEventStreamCreate(NULL,
    &cb,
    NULL,                           /* callbackInfo */
    paths,
    kFSEventStreamEventIdSinceNow,  /* only changes from now on */
    latency,
    kFSEventStreamCreateFlagNoDefer /* Flags explained in reference */
  );

  if (signal(SIGINT, graceful_shutdown) == SIG_ERR)
    err(1, "signal");
  if (signal(SIGTERM, graceful_shutdown) == SIG_ERR)
    err(1, "signal");

  FSEventStreamScheduleWithRunLoop(stream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

  /* start! */
  FSEventStreamStart(stream);
  CFRunLoopRun();

  /* should not reach */
  exit(2);
}

void cb(
    ConstFSEventStreamRef stream_ref,
    void *client_cbinfo,
    size_t num_events,
    void *event_paths,
    const FSEventStreamEventFlags event_flags[],
    const FSEventStreamEventId event_ids[])
{
  int i;
  char **paths = event_paths;

  for (i = 0; i < num_events; i++) {
    printf("%s\n", paths[i]);
    if (fflush(stdout) == EOF)
      err(1, "fflush");
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
