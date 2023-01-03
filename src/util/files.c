/* wl-clipboard
 *
 * Copyright © 2019 Sergey Bugaev <bugaevc@gmail.com>
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

#include <util/files.h>
#include <util/string.h>
#include <util/misc.h>

#include "config.h"

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <string.h>
#include <fcntl.h> // open
#include <sys/sendfile.h> // sendfile
#include <sys/stat.h> // open
#include <sys/types.h> // open
#include <stdlib.h> // exit
#include <libgen.h> // basename
#include <sys/wait.h>
#include <sys/mman.h>

#ifdef HAVE_MEMFD
#    include <sys/syscall.h> // syscall, SYS_memfd_create
#endif
#ifdef HAVE_SHM_ANON
#    include <sys/mman.h> // shm_open, SHM_ANON
#endif
#ifdef HAVE_CLONE3
#    include <linux/sched.h>
#endif


static void do_close(struct anonfile* file) {
    if (close(file->fd)) {
        perror("tmpfile/destroy: close");
    }
}

static void do_fclose(struct anonfile* file) {
    FILE* ctx = file->ctx;
    if (fclose(ctx)) {
        perror("tmpfile/destroy: fclose");
    }
}


int create_anonymous_file(struct anonfile* file) {
    int res;
#ifdef HAVE_MEMFD
    res = syscall(SYS_memfd_create, "buffer", 0);
    if (res >= 0) {
        file->destroy = do_close;
        file->ctx = NULL;
        file->fd = res;
        return 0;
    }
#endif
#ifdef HAVE_SHM_ANON
    res = shm_open(SHM_ANON, O_RDWR | O_CREAT, 0600);
    if (res >= 0) {
        file->destroy = do_close;
        file->ctx = NULL;
        file->fd = res;
        return 0;
    }
#endif

    FILE* tmp = tmpfile();
    if (!tmp) {
        perror("tmpfile");
        return -1;
    }

    res = fileno(tmp);
    if (res < 0) {
        fclose(tmp);
        perror("fileno");
        return res;
    }
    file->destroy = do_fclose;
    file->ctx = tmp;
    file->fd = res;
    return 0;
}

void trim_trailing_newline(const char *file_path) {
    int fd = open(file_path, O_RDWR);
    if (fd < 0) {
        perror("open file for trimming");
        return;
    }

    int seek_res = lseek(fd, -1, SEEK_END);
    if (seek_res < 0 && errno == EINVAL) {
        /* It was an empty file */
        goto out;
    } else if (seek_res < 0) {
        perror("lseek");
        goto out;
    }
    /* If the seek was successful, seek_res is the
     * new file size after trimming the newline.
     */
    char last_char;
    int read_res = read(fd, &last_char, 1);
    if (read_res != 1) {
        perror("read");
        goto out;
    }
    if (last_char != '\n') {
        goto out;
    }

    int rc = ftruncate(fd, seek_res);
    if (rc < 0) {
        perror("ftruncate");
    }
out:
    close(fd);
}

char *path_for_fd(int fd) {
    char fdpath[64];
    snprintf(fdpath, sizeof(fdpath), "/dev/fd/%d", fd);
    return realpath(fdpath, NULL);
}

char *infer_mime_type_from_contents(const char *file_path) {
    /* Spawn xdg-mime query filetype */
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
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        int devnull = open("/dev/null", O_RDONLY);
        if (devnull >= 0) {
            dup2(devnull, STDIN_FILENO);
            close(devnull);
        } else {
            /* If we cannot open /dev/null, just close stdin */
            close(STDIN_FILENO);
        }
        execlp("xdg-mime", "xdg-mime", "query", "filetype", file_path, NULL);
        exit(1);
    }

    close(pipefd[1]);
    int wstatus;
    wait(&wstatus);

    /* See if that worked */
    if (!WIFEXITED(wstatus) || WEXITSTATUS(wstatus) != 0) {
        close(pipefd[0]);
        return NULL;
    }

    /* Read the result */
    char *res = malloc(256);
    size_t len = read(pipefd[0], res, 256);
    /* Trim the newline */
    len--;
    res[len] = 0;
    close(pipefd[0]);

    if (str_has_prefix(res, "inode/")) {
        free(res);
        return NULL;
    }

    return res;
}

char *infer_mime_type_from_name(const char *file_path) {
    const char *actual_ext = get_file_extension(file_path);
    if (actual_ext == NULL) {
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
        /* Skip comments and black lines */
        if (line[0] == '#' || line[0] == '\n') {
            continue;
        }

        /* Each line consists of a mime type and a list of extensions */
        char mime_type[200];
        int consumed;
        if (sscanf(line, "%199s%n", mime_type, &consumed) != 1) {
            /* A malformed line, perhaps? */
            continue;
        }
        char *lineptr = line + consumed;
        for (char ext[200]; sscanf(lineptr, "%199s%n", ext, &consumed) == 1;) {
            if (strcmp(ext, actual_ext) == 0) {
                fclose(f);
                return strdup(mime_type);
            }
            lineptr += consumed;
        }
    }
    fclose(f);
    return NULL;
}

static int dump_stdin_to_file_using_cat(const char* res_path) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }
    if (pid == 0) {
        int fd = open(res_path, O_RDWR | O_EXCL | O_CREAT, S_IRUSR | S_IWUSR);
        if (fd < 0) {
            perror("open");
            exit(1);
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
        execlp("cat", "cat", NULL);
        perror("exec cat");
        exit(1);
    }

    int wstatus;
    wait(&wstatus);
    if (!WIFEXITED(wstatus) || WEXITSTATUS(wstatus) != 0) {
        return -1;
    }
    return 0;
}

static int dump_stdin_to_file_using_mmap(int fd) {
    struct buffer mmapped;
    int err = buffer_mmap_file(&mmapped, STDIN_FILENO);
    if (err) {
        return err;
    }

    char* ptr = mmapped.ptr;
    size_t len = mmapped.len;
    while (len) {
        ssize_t written = write(fd, ptr, len);
        if (written < 0) {
            perror("dump_stdin_to_file_using_mmap: write");
            return -1;
        }
        len -= written;
        ptr += written;
    }
    return 0;
}

static int dump_stdin_to_file_using_sendfile(int fd) {
    ssize_t bytes = sendfile(fd, STDIN_FILENO, NULL, 4096);
    if (bytes < 0) {
        int err = errno;
        if (err != EINVAL) {
            perror("sendfile");
            return -1;
        }
        return 1;
    }
    while (bytes > 0) {
        bytes = sendfile(fd, STDIN_FILENO, NULL, 4096);
    }
    if (bytes < 0) {
        perror("sendfile");
        return -1;
    }

    return 0;
}

/* Returns the name of a new file */
int dump_stdin_into_a_temp_file(int* fildes, char** path) {
    /* Create a temp directory to host out file */
    char dirpath[] = "/tmp/wl-copy-buffer-XXXXXX";
    if (mkdtemp(dirpath) != dirpath) {
        perror("mkdtemp");
        return -1;
    }

    /* Pick a name for the file we'll be
     * creating inside that directory. We
     * try to preserve the origial name for
     * the mime type inference to work.
     */
    char *original_path = path_for_fd(STDIN_FILENO);
    char *name = original_path != NULL ? basename(original_path) : "stdin";

    /* Construct the path */
    char *res_path = malloc(strlen(dirpath) + 1 + strlen(name) + 1);
    memcpy(res_path, dirpath, sizeof(dirpath));
    res_path[sizeof(dirpath) - 1] = '/';
    strcpy(res_path + sizeof(dirpath), name);

    int fd = open(res_path, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    // Try hard to do less, forking is much more expensive anyway
    int err = dump_stdin_to_file_using_mmap(fd);
    switch (err) {
        case 0: goto done;
        case -1: goto fail;
    }

    err = dump_stdin_to_file_using_sendfile(fd);
    switch (err) {
        case 0: goto done;
        case -1: goto fail;
    }

    err = dump_stdin_to_file_using_cat(res_path);
    switch (err) {
        case 0: goto done;
        case -1: goto fail;
    }

fail:
    close(fd);
    unlink(res_path);
    rmdir(dirpath);
    return -1;

done:
    if (original_path != NULL) {
        free(original_path);
    }
    *fildes = fd;
    *path = res_path;
    return 0;
}


static void do_munmap(struct buffer* self) {
    if (munmap(self->ptr, self->len) ) {
        perror("owned_slice/destroy: do_munmap");
    }
    memset(self, 0, sizeof(*self));
}


int buffer_mmap_file(struct buffer* self, int fd) {
    struct stat stat;
    if (fstat(fd, &stat)) {
        perror("owned_slice_mmap_file: fstat");
        return -1;
    }

    if (!S_ISREG(stat.st_mode)) {
        return 1;
    }

    char* ptr = mmap(NULL, stat.st_blksize, MAP_PRIVATE, PROT_READ, fd, 0);
    if (ptr == MAP_FAILED) {
        return 1;
    }

    self->destroy = do_munmap;
    self->ptr = ptr;
    self->len = stat.st_blksize;
    return 0;
}



static void do_free(struct buffer* self) {
    free(self->ptr);
}

enum {
    BUFFER_SIZE = 4096
};

int copy_stdin_to_mem(struct buffer* slice) {
    // opportunistic mmap in case if it's a mmappable (e.g. wl-copy < some_file)
    switch (buffer_mmap_file(slice, STDIN_FILENO)) {
        case -1: return -1;
        case 0: return 0;
        case 1: break;
    }

    char* begin = malloc(BUFFER_SIZE);
    if (!begin) {
        goto fail;
    }
    char* ptr = begin;
    char* end = begin + BUFFER_SIZE;

    for (;;) {
        ssize_t bytes = read(STDIN_FILENO, ptr, end - ptr);
        if (bytes < 0) {
            perror("copy_stdin_to_mem: read");
            goto fail;
        }
        if (bytes == 0) {
            break;
        }

        ptr += bytes;
        if (ptr == end) {
            size_t len = end - begin;
            begin = realloc(begin, len + BUFFER_SIZE);
            if (!begin) {
                goto fail;
            }
            ptr = begin + len;
            end = begin + len + BUFFER_SIZE;
        }
    }

    slice->destroy = do_free;
    slice->ptr = begin;
    slice->len = ptr - begin;

    return 0;

fail:
    if (begin) {
        free(begin);
    }
    return -1;
}


static void sensitive_do_close(struct sensitive* self) {
    struct sensitive_fd* self2 = (struct sensitive_fd*)self;
    int fd = self2->fd;
    self2->fd = -1;
    close(fd);
}

void sensitive_fd_init(struct sensitive_fd* handler, int fd) {
    handler->impl.cleanup = sensitive_do_close;
    handler->fd = fd;
}

