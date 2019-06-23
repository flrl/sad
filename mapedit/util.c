#include <config.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/stat.h>

#include "mapedit/util.h"

#define KEEP_OLD_VERSIONS (10)

static char *get_backup_filename(const char *orig_filename)
{
    const size_t alloc = strlen(orig_filename) + 5 + 1;
    char *backup_filename = NULL;
    char *ext;
    struct stat stat_buf;
    struct timespec oldest_time;
    int i, r, oldest;

    backup_filename = malloc(alloc);
    if (!backup_filename) return NULL;

    memset(backup_filename, 0, alloc);
    snprintf(backup_filename, alloc, "%s.bak", orig_filename);
    ext = strrchr(backup_filename, '\0');

    for (i = 0; i < KEEP_OLD_VERSIONS; i++) {
        *ext = '0' + i;
        r = stat(backup_filename, &stat_buf);
        if (r && errno == ENOENT)
            return backup_filename;
    }

    /* all candidate names already in use, reuse the oldest one */
    oldest_time.tv_sec = time(0) + 1; /* start in the future */
    oldest_time.tv_nsec = 0;
    oldest = -1;
    for (i = 0; i < KEEP_OLD_VERSIONS; i++) {
        *ext = '0' + i;
        r = stat(backup_filename, &stat_buf);
        if (r) continue;

/* XXX i think st_mtim is the modern portable name for this */
#ifdef HAVE_STRUCT_STAT_ST_MTIMESPEC
        if ((stat_buf.st_mtimespec.tv_sec < oldest_time.tv_sec) ||
            (stat_buf.st_mtimespec.tv_sec == oldest_time.tv_sec &&
             stat_buf.st_mtimespec.tv_nsec < oldest_time.tv_nsec)) {
            oldest_time = stat_buf.st_mtimespec;
            oldest = i;
        }
#else
        if (stat_buf.st_mtime < oldest_time.tv_sec) {
            oldest_time.tv_sec = stat_buf.st_mtime;
            oldest = i;
        }
#endif
    }

    /* found an oldest one */
    if (oldest != -1) {
        *ext = '0' + oldest;
        return backup_filename;
    }

    /* they were all the same age, to the nanosecond?? */
    free(backup_filename);
    return NULL;
}

FILE *fopen_with_backup(const char *filename)
{
    FILE *file = NULL;
    char *backup_filename = NULL;
    struct stat stat_buf;
    int fd, r;

    r = stat(filename, &stat_buf);
    if (r && errno != ENOENT) return NULL;

    if (!r) {
        // filename exists, move it out of the way first
        backup_filename = get_backup_filename(filename);
        if (!backup_filename) return NULL;
        r = rename(filename, backup_filename);
        free(backup_filename);
        if (r) return NULL;
    }

    fd = open(filename, O_CREAT|O_EXCL|O_WRONLY,
              S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
    if (fd < 0) return NULL;

    file = fdopen(fd, "w");
    if (!file) close(fd);

    return file;
}
