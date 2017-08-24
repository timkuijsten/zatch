#include <sys/stat.h>
#include <err.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include <CoreServices/CoreServices.h>

extern char *__progname;

FSEventStreamRef stream;

struct path_map {
  int resolvedlen; /* length of resolved path, excluding the terminating null byte */
  char *resolved;
  int origlen;
  char *orig;
};

void cb(ConstFSEventStreamRef, void *, size_t numEvents, void *, const FSEventStreamEventFlags *, const FSEventStreamEventId *);
void graceful_shutdown(int);

static int debug = 0;
static struct path_map **path_maps;

int print_usage(FILE *fp);

/* options */
static int preflight, subdir;

int
main(int argc, char *argv[])
{
  int i;
  CFStringRef *tmp_path, *pp;
  CFArrayRef paths;
  CFAbsoluteTime latency = 0.03; /* default latency in seconds */
  char resolved[PATH_MAX + 1], c, *p;
  struct path_map **pm;
  struct stat st;

  preflight = 0;
  subdir    = 0;

  while ((c = getopt(argc, argv, "hpsdl:")) != -1) {
    switch (c) {
    case 'd':
      debug = 1;
      break;
    case 'l':
      if ((latency = strtod(optarg, &p)) == 0)
        err(1, "strtol");
      break;
    case 'p':
      preflight = 1;
      break;
    case 's':
      subdir = 1;
      break;
    case 'h':
      print_usage(stdout);
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

  if ((tmp_path = calloc(argc, sizeof(CFStringRef))) == NULL)
    err(1, "calloc CFStringRef");

  /* first allocate enough space for a pointer to a structure for each dir plus a NULL pointer */
  if ((path_maps = (struct path_map **)calloc(argc + 1, sizeof(struct path_map **))) == NULL)
    err(1, "calloc path_map **");
  pm = path_maps;

  /* make sure each parameter really is an existing directory */
  pp = tmp_path;
  if (debug)
    fprintf(stderr, "watching");
  for (i = 0; i < argc; i++)
    if (stat(argv[i], &st) == -1) {
      err(1, "%s", argv[i]);
    } else {
      if (!S_ISDIR(st.st_mode))
        errx(1, "%s is not a directory", argv[i]);

      if (realpath(argv[i], resolved) == NULL)
        err(1, "realpath");
      if ((*pm = malloc(sizeof(struct path_map))) == NULL)
        err(1, "malloc path_map");
      (*pm)->resolvedlen = strlen(resolved);
      if (((*pm)->resolved = calloc((*pm)->resolvedlen + 1, sizeof(char))) == NULL)
        err(1, "calloc strlen");
      strcpy((*pm)->resolved, resolved);
      (*pm)->origlen = strlen(argv[i]);
      (*pm)->orig = argv[i];
      pm++;

      if ((*pp++ = CFStringCreateWithCString(NULL, resolved, kCFStringEncodingUTF8)) == NULL)
        errx(1, "CFStringCreateWithCString");
      if (debug)
        fprintf(stderr, " %s", resolved);
    }

  /* signal path map end */
  *pm = NULL;

  if (debug)
    fprintf(stderr, "\n");

  paths = CFArrayCreate(NULL, (const void **)tmp_path, argc, NULL);

  stream = FSEventStreamCreate(NULL,
    &cb,
    NULL,                           /* callback info */
    paths,
    kFSEventStreamEventIdSinceNow,  /* only changes from now on */
    latency,
    kFSEventStreamCreateFlagNone
  );

  if (signal(SIGINT, graceful_shutdown) == SIG_ERR)
    err(1, "signal");
  if (signal(SIGTERM, graceful_shutdown) == SIG_ERR)
    err(1, "signal");

  FSEventStreamScheduleWithRunLoop(stream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

  if (preflight)
    for (pm = path_maps; *pm; pm++) {
      if (subdir && (*pm)->orig[(*pm)->origlen - 1] != '/') /* ensure trailing slash */
        fprintf(stdout, "%s/\n", (*pm)->orig);
      else
        fprintf(stdout, "%s\n", (*pm)->orig);
      if (fflush(stdout) == EOF)
        err(1, "fflush");
    }

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
  int i, soff; /* string offset */
  char **paths = event_paths;
  struct path_map **pm;

  for (i = 0; i < num_events; i++) {
    if (debug && event_flags[i] != kFSEventStreamEventFlagNone)
      fprintf(stderr, "flags present: %x\n", event_flags[i]);

    /* translate to user supplied path, optionally with subdir */
    for (pm = path_maps; *pm; pm++) {
      if (strncmp((*pm)->resolved, paths[i], (*pm)->resolvedlen) != 0)
        continue;

      fprintf(stdout, "%s", (*pm)->orig);
      if (subdir) {
        /* resolved is excluding a trailing slash unless the root of the file-
         * system is watched, paths is including a trailing slash, orig is user
         * defined */
        soff = (*pm)->resolvedlen; /* expect paths[i][soff] == "/" unless orig is the root */
        /* step over the slash if the user supplied a dir with a trailing slash
         * unless it's the file-system root */
        if ((*pm)->resolvedlen > 1 && (*pm)->orig[(*pm)->origlen - 1] == '/')
          soff++;
        fprintf(stdout, "%s", &paths[i][soff]);
      }
      fprintf(stdout, "\n");

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

/* return number of characters printed or <0 on error */
int
print_usage(FILE *fp)
{
  return fprintf(fp, "usage: %s [-hps] dir ...\n", __progname);
}
