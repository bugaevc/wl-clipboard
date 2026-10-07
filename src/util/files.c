/* wl-clipboard
 *
 * Copyright © 2018-2026 Sergey Bugaev <bugaevc@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "util/files.h"
#include "util/string.h"
#include "util/misc.h"

#include "config.h"

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <string.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <sys/stat.h> // open
#include <sys/types.h> // open
#include <stdlib.h> // exit
#include <libgen.h> // basename
#include <sys/wait.h>
#include <signal.h>
#include <syslog.h>

#ifdef HAVE_MEMFD
#    include <sys/syscall.h> // syscall, SYS_memfd_create
#endif
#ifdef HAVE_SHM_ANON
#    include <sys/mman.h> // shm_open, SHM_ANON
#endif

#include <wayland-client.h> // wl_display_get_fd


void complain_about_closed_stdio(struct wl_display *wl_display) {
    const char *message = "wl-clipboard has been launched with a closed"
        " standard file descriptor. This is a bug in the software that"
        " has launched wl-clipboard. Aborting.";

    /* See if we can write to stderr */
    if (wl_display_get_fd(wl_display) < STDERR_FILENO) {
        int rc = fcntl(STDERR_FILENO, F_GETFL);
        if (rc > 0) {
            rc &= O_ACCMODE;
            if (rc == O_WRONLY || rc == O_RDWR) {
                /* Yes, we can! */
                fprintf(stderr, "%s\n", message);
                fflush(stderr);
                abort();
            }
        }
    }

    /* Maybe there is a tty we could write to? */
    FILE *tty = fopen("/dev/tty", "w");
    if (tty != NULL) {
        fprintf(tty, "%s\n", message);
        fflush(tty);
        abort();
    }

    /* As a last resort, try syslog */
    openlog("wl-clipboard", LOG_CONS | LOG_PID, LOG_USER);
    syslog(LOG_ERR, "%s", message);
    closelog();
    abort();
}

int create_anonymous_file() {
    int res;
#ifdef HAVE_MEMFD
    res = syscall(SYS_memfd_create, "buffer", 0);
    if (res >= 0) {
        return res;
    }
#endif
#ifdef HAVE_SHM_ANON
    res = shm_open(SHM_ANON, O_RDWR | O_CREAT, 0600);
    if (res >= 0) {
        return res;
    }
#endif
    (void) res;
    return fileno(tmpfile());
}

int copy_fd(int src, int dest) {
    /* Copy data between two file descriptors */

    char *buf[256]; /* Arbitrary buffer size, can be made a macro */
    ssize_t rlen, wlen, written;

    while ((rlen = read(src, buf, 256)) > 0) {
        for (written = 0; written < rlen; written += wlen) {
            if ((wlen = write(dest, buf + written, rlen - written)) < 0) {
                return 1;
            }
        }
    }
    if (rlen < 0) {
        return 1;
    }
    return 0;
}

void trim_trailing_newline(int fd) {
    int seek_res = lseek(fd, -1, SEEK_END);
    if (seek_res < 0 && errno == EINVAL) {
        /* It was an empty file */
        return;
    } else if (seek_res < 0) {
        perror("lseek");
        return;
    }
    /* If the seek was successful, seek_res is the
     * new file size after trimming the newline.
     */
    char last_char;
    int read_res = read(fd, &last_char, 1);
    if (read_res != 1) {
        perror("read");
        return;
    }
    if (last_char != '\n') {
        return;
    }

    int rc = ftruncate(fd, seek_res);
    if (rc < 0) {
        perror("ftruncate");
    }
}

char *path_for_fd(int fd) {
    char fdpath[64];
    snprintf(fdpath, sizeof(fdpath), "/dev/fd/%d", fd);
    return realpath(fdpath, NULL);
}

char *infer_mime_type_from_contents(int fd) {
    /* Spawn file(1) to check filetype */
    if (lseek(fd, 0, SEEK_SET) != 0) {
        perror("lseek");
        return NULL;
    }

    int pipefd[2];
    int rc = pipe(pipefd);
    if (rc < 0) {
        perror("pipe");
        return NULL;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        close(pipefd[0]);
        close(pipefd[1]);
        return NULL;
    }
    if (pid == 0) {
        dup2(fd, STDIN_FILENO);
        close(fd);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        signal(SIGHUP, SIG_DFL);
        signal(SIGPIPE, SIG_DFL);
        execlp("file", "file", "--brief", "--mime-type", "-", (char *) NULL);
        exit(1);
    }

    close(pipefd[1]);
    int wstatus;
    waitpid(pid, &wstatus, 0);

    /* See if that worked */
    if (!WIFEXITED(wstatus) || WEXITSTATUS(wstatus) != 0) {
        close(pipefd[0]);
        return NULL;
    }

    /* Read the result */
    char *res = malloc(256);
    ssize_t len = read(pipefd[0], res, 256);
    close(pipefd[0]);
    if (len <= 0) {
        free(res);
        return NULL;
    }
    /* Trim the newline */
    len--;
    res[len] = 0;

    if (str_has_prefix(res, "inode/")) {
        free(res);
        return NULL;
    }

    return res;
}

static char *search_mime_dot_types_for_ext(const char *ext) {
    if (ext == NULL) {
        return NULL;
    }

    FILE *f = fopen("/etc/mime.types", "r");
    if (f == NULL) {
        f = fopen("/usr/local/etc/mime.types", "r");
    }
    if (f == NULL) {
        return NULL;
    }

    for (char line[200]; fgets(line, sizeof(line), f) != NULL;) {
        /* Skip comments and blank lines */
        if (line[0] == '#' || line[0] == '\n') {
            continue;
        }

        /* Each line consists of a mime type and a list of extensions */
        char mime_type[200];
        int consumed;
        if (sscanf(line, "%199s%n", mime_type, &consumed) != 1) {
            /* A malformed line, perhaps? */
            fprintf(stderr, "malformed mime.types line: %s\n", line);
            continue;
        }
        char *lineptr = line + consumed;
        for (char ext_pattern[200]; sscanf(lineptr, "%199s%n", ext_pattern, &consumed) == 1;) {
            if (strcmp(ext_pattern, ext) == 0) {
                fclose(f);
                return strdup(mime_type);
            }
            lineptr += consumed;
        }
    }
    fclose(f);
    return NULL;
}

static char *search_shared_mime_info_globs_for_filename(const char *filename) {
    FILE *f = fopen("/usr/share/mime/globs2", "r");
    if (f == NULL) {
        f = fopen("/usr/local/share/mime/globs2", "r");
    }
    if (f == NULL) {
        return NULL;
    }

    for (char line[200]; fgets(line, sizeof(line), f) != NULL;) {
        /* Skip comments and blank lines */
        if (line[0] == '#' || line[0] == '\n') {
            continue;
        }

        /* Each line consists of colon-separated
         * weight, mime type, and glob pattern.
         * We ignore the weight.
         */
        char mime_type[200];
        char filename_glob[200];
        if (sscanf(line, "%*d:%199[^:]:%199s\n", mime_type, filename_glob) != 2) {
            /* A malformed line, perhaps? */
            fprintf(stderr, "malformed globs2 line: %s\n", line);
            continue;
        }
        if (fnmatch(filename_glob, filename, 0) == 0) {
            fclose(f);
            return strdup(mime_type);
        }
    }
    fclose(f);
    return NULL;
}

char *infer_mime_type_from_name(const char *file_path) {
    const char *ext = get_file_extension(file_path);
    char *file_path_dup = strdup(file_path);
    const char *filename = basename(file_path_dup);
    char *mime_type = search_shared_mime_info_globs_for_filename(filename);
    if (!mime_type) {
        mime_type = search_mime_dot_types_for_ext(ext);
    }
    free(file_path_dup);
    return mime_type;
}
