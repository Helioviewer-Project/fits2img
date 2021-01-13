/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

static const char _versionid_[] __attribute__((unused)) =
    "$Id: p2sc_file.c 5108 2014-06-19 12:29:23Z bogdan $";

#define _XOPEN_SOURCE 600
#include <sys/stat.h>
#include <fcntl.h>
#include <ftw.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <glib.h>

#include "p2sc_file.h"
#include "p2sc_msg.h"
#include "p2sc_stdlib.h"

struct p2sc_keyfile_t {
    GKeyFile *key;
    char *name;
};

p2sc_keyfile_t *p2sc_open_keyfile(const char *file) {
    GError *err = NULL;
    p2sc_keyfile_t *k = (p2sc_keyfile_t *) g_malloc(sizeof *k);

    k->name = g_strdup(file);
    k->key = g_key_file_new();
    if (!g_key_file_load_from_file(k->key, k->name, (GKeyFileFlags) 0, &err) || err) {
        if (err && err->message) {
            P2SC_Msg(LVL_FATAL, "%s: %s", k->name, err->message);
            g_error_free(err);
        } else
            P2SC_Msg(LVL_FATAL, "%s: key file load failed - unknown reason", k->name);
        /* not reached */
        p2sc_free_keyfile(k);
        k = NULL;
    }
    return k;
}

void p2sc_free_keyfile(p2sc_keyfile_t * k) {
    if (k) {
        if (k->key)
            g_key_file_free(k->key);
        g_free(k->name);
        memset(k, 0, sizeof *k);
        g_free(k);
    }
}

char *p2sc_get_keyvalue(p2sc_keyfile_t * k, const char *group, const char *key) {
    GError *err = NULL;
    char *ret = g_key_file_get_value(k->key, group, key, &err);

    if (!ret || err) {
        if (err && err->message) {
            P2SC_Msg(LVL_FATAL, "%s: %s", k->name, err->message);
            g_error_free(err);
        } else
            P2SC_Msg(LVL_FATAL, "%s: value of %s not found in group %s", k->name, key, group);
        /* not reached */
        g_free(ret);
        ret = NULL;
    }
    return ret;
}

char *p2sc_get_temp(const char *key) {
    p2sc_keyfile_t *kf = p2sc_open_keyfile(P2SC_GLOBAL_INI);

    char *top = p2sc_get_keyvalue(kf, "dir", "top");
    char *dir = p2sc_get_keyvalue(kf, "dir", key);
    const char *rid = p2sc_get_string("runid");
    char *tmp = g_build_filename(top, dir, rid, NULL);

    if (g_mkdir_with_parents(tmp, 0755))
        P2SC_Msg(LVL_FATAL_FILESYSTEM, "mkdir(%s): %s", tmp, g_strerror(errno));

    g_free(dir);
    g_free(top);

    return tmp;
}

struct p2sc_iofile_t {
    GIOChannel *io;
    char *name;
    size_t lineno;
};

p2sc_iofile_t *p2sc_open_iofile(const char *name, const char *mode) {
    GError *err = NULL;
    p2sc_iofile_t *f = (p2sc_iofile_t *) g_malloc(sizeof *f);

    if (name[0] == '-') {
        if (mode[0] == 'r') {
            f->name = g_strdup("stdin");
            f->io = g_io_channel_unix_new(STDIN_FILENO);
        } else if (mode[0] == 'w' || mode[0] == 'a') {
            f->name = g_strdup("stdout");
            f->io = g_io_channel_unix_new(STDOUT_FILENO);
        } else
            P2SC_Msg(LVL_FATAL, "Unknown mode %s on %s", mode, name);
    } else {
        f->name = g_strdup(name);
        f->io = g_io_channel_new_file(f->name, mode, &err);
    }

    if (f->io) {
        GIOStatus status = g_io_channel_set_encoding(f->io, NULL, &err);
        if (err && err->message) {
            P2SC_Msg(LVL_FATAL_FILESYSTEM, "%s: failed to change encoding of IO channel: %s",
                     f->name, err->message);
            g_error_free(err);
            /* not reached */
            p2sc_free_iofile(f);
            return NULL;
        } else if (status != G_IO_STATUS_NORMAL) {
            P2SC_Msg(LVL_FATAL_FILESYSTEM,
                     "%s: failed to change encoding of IO channel with status %d", f->name,
                     (int) status);
            /* not reached */
            p2sc_free_iofile(f);
            return NULL;
        }
    } else {
        if (err && err->message) {
            P2SC_Msg(LVL_FATAL_FILESYSTEM, "%s: failed to open IO channel: %s",
                     f->name, err->message);
            g_error_free(err);
        } else
            P2SC_Msg(LVL_FATAL_FILESYSTEM,
                     "%s: failed to open IO channel: unknown reason", f->name);
        /* not reached */
        p2sc_free_iofile(f);
        return NULL;
    }

    g_io_channel_set_close_on_unref(f->io, TRUE);
    f->lineno = 0;

    return f;
}

void p2sc_free_iofile(p2sc_iofile_t * f) {
    if (f) {
        if (f->io)
            g_io_channel_unref(f->io);
        g_free(f->name);
        memset(f, 0, sizeof *f);
        g_free(f);
    }
}

char *p2sc_read_line(p2sc_iofile_t * f) {
    gsize term;
    char *line = NULL;
    GError *err = NULL;
    GIOStatus status = g_io_channel_read_line(f->io, &line, NULL, &term, &err);

    if (err && err->message) {
        g_free(line);
        line = NULL;
        P2SC_Msg(LVL_FATAL_FILESYSTEM, "%s:%zd: error reading: %s", f->name,
                 f->lineno, err->message);
        g_error_free(err);
    }

    if (status != G_IO_STATUS_NORMAL && status != G_IO_STATUS_EOF) {
        g_free(line);
        line = NULL;
        P2SC_Msg(LVL_FATAL_FILESYSTEM, "%s:%zd: read failed with status: %d",
                 f->name, f->lineno, (int) status);
    }

    if (line && (line[term] == '\n' || line[term] == '\r'))
        line[term] = 0;

    f->lineno++;

    return line;
}

size_t p2sc_get_lineno(p2sc_iofile_t * f) {
    return f->lineno;
}

void p2sc_read(p2sc_iofile_t * f, char *buf, gsize count) {
    gsize bytes_read;
    GError *err = NULL;
    GIOStatus status = g_io_channel_read_chars(f->io, buf, count, &bytes_read, &err);

    if (err && err->message) {
        P2SC_Msg(LVL_FATAL_FILESYSTEM, "%s:%zd: error reading: %s", f->name,
                 f->lineno, err->message);
        g_error_free(err);
    }

    if (status != G_IO_STATUS_NORMAL)
        P2SC_Msg(LVL_FATAL_FILESYSTEM,
                 "%s:%zd: error reading with status %d", f->name, f->lineno, (int) status);

    if (bytes_read != count)
        P2SC_Msg(LVL_FATAL_FILESYSTEM,
                 "%s:%zd: error reading: expected %zd, read %zd", f->name, f->lineno, count,
                 bytes_read);
}

void p2sc_write(p2sc_iofile_t * f, const char *buf, gssize len) {
    if (len < 0)
        len = strlen(buf);

    gsize count;
    GError *err = NULL;
    GIOStatus status = g_io_channel_write_chars(f->io, buf, len, &count, &err);

    if (err && err->message) {
        P2SC_Msg(LVL_FATAL_FILESYSTEM, "%s:%zd: error writing: %s", f->name,
                 f->lineno, err->message);
        g_error_free(err);
    }

    if (status != G_IO_STATUS_NORMAL)
        P2SC_Msg(LVL_FATAL_FILESYSTEM,
                 "%s:%zd: error writing with status %d", f->name, f->lineno, (int) status);

    if (count != (gsize) len)
        P2SC_Msg(LVL_FATAL_FILESYSTEM,
                 "%s:%zd: error writing %zd bytes: %zd written", f->name,
                 f->lineno, count, (gsize) len);
}

void p2sc_flush(p2sc_iofile_t * f) {
    GError *err = NULL;
    GIOStatus status = g_io_channel_flush(f->io, &err);

    if (err && err->message) {
        P2SC_Msg(LVL_FATAL_FILESYSTEM, "%s:%zd: error flushing: %s", f->name,
                 f->lineno, err->message);
        g_error_free(err);
    }

    if (status != G_IO_STATUS_NORMAL)
        P2SC_Msg(LVL_FATAL_FILESYSTEM,
                 "%s:%zd: error flushing with status %d", f->name, f->lineno, (int) status);
}

char *p2sc_test_file_input(const char *dir, const char *file) {
    char *name = NULL;

    if (g_file_test(dir, G_FILE_TEST_IS_DIR)) {
        if (access(dir, R_OK))
            P2SC_Msg(LVL_FATAL_FILESYSTEM, "Input directory %s is not readable", dir);
        else
            name = g_build_filename(dir, file, NULL);
    } else
        P2SC_Msg(LVL_FATAL_FILESYSTEM,
                 "Input directory %s does not exist or is not a directory", dir);

    if (access(name, R_OK))
        P2SC_Msg(LVL_FATAL_FILESYSTEM, "Input file %s does not exist or is not readable", name);

    return name;
}

char *p2sc_test_file_output(const char *dir, const char *file) {
    char *name = NULL;

    if (g_file_test(dir, G_FILE_TEST_IS_DIR)) {
        if (access(dir, W_OK))
            P2SC_Msg(LVL_FATAL_FILESYSTEM, "Output directory %s is not writable", dir);
        else
            name = g_build_filename(dir, file, NULL);
    } else
        P2SC_Msg(LVL_FATAL_FILESYSTEM,
                 "Output directory %s does not exist or is not a directory", dir);

    if (g_file_test(name, G_FILE_TEST_IS_REGULAR)) {
        if (access(name, W_OK))
            P2SC_Msg(LVL_FATAL_FILESYSTEM, "Output file %s is not writable", name);
    }

    return name;
}

char **p2sc_readlines_file(const char *name, int strip) {
    char *line;
    p2sc_iofile_t *f = p2sc_open_iofile(name, "r");
    GArray *a = g_array_new(TRUE, FALSE, sizeof(char *));

    while ((line = p2sc_read_line(f))) {
        if (strip) {
            line = g_strstrip(line);
            if (!*line) {
                g_free(line);
                continue;
            }
        }
        g_array_append_val(a, line);
    }

    p2sc_free_iofile(f);

    return (char **) g_array_free(a, FALSE);
}

void p2sc_strip_strings(char ***sss) {
    char **ss = *sss, *s;
    guint i, n = g_strv_length(ss);
    GArray *a = g_array_sized_new(TRUE, FALSE, sizeof(char *), n);

    for (i = 0; i < n; ++i) {
        s = g_strstrip(ss[i]);
        if (!*s)
            g_free(s);
        else
            g_array_append_val(a, s);
    }
    g_free(ss);
    *sss = (char **) g_array_free(a, FALSE);
}

int p2sc_create_file(int nuke, const char *name, const void *buf, size_t nbyte) {
    int fd, flags = O_CREAT | O_TRUNC | O_WRONLY;

    if (!nuke)
        flags |= O_EXCL;

    fd = open(name, flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd == -1) {
        if (errno == EEXIST)
            return -1;
        else
            P2SC_Msg(LVL_FATAL_FILESYSTEM, "open(%s): %s", name, g_strerror(errno));
    }

    if (write(fd, buf, nbyte) == -1 || fsync(fd) == -1 || close(fd) == -1) {
        int e = errno;

        unlink(name);
        P2SC_Msg(LVL_FATAL_FILESYSTEM, "%s: %s", name, g_strerror(e));
    }

    return 0;
}

void p2sc_copy_file(const char *from, const char *to) {
    GError *err = NULL;
    GMappedFile *map = p2sc_map_file(from);

    char *dir = g_path_get_dirname(to);
    if (g_mkdir_with_parents(dir, 0755))
        P2SC_Msg(LVL_FATAL_FILESYSTEM, "mkdir(%s): %s", dir, g_strerror(errno));
    g_free(dir);

    if (!g_file_set_contents(to,
                             g_mapped_file_get_contents(map),
                             g_mapped_file_get_length(map), &err)) {
        if (err && err->message) {
            P2SC_Msg(LVL_FATAL_FILESYSTEM, "%s", err->message);
            g_error_free(err);
        } else
            P2SC_Msg(LVL_FATAL_FILESYSTEM, "%s: file writing failed: unknown reason", to);
    }
    g_mapped_file_unref(map);
}

GMappedFile *p2sc_map_file(const char *name) {
    GError *err = NULL;
    GMappedFile *map = g_mapped_file_new(name, FALSE, &err);

    if (!map) {
        if (err && err->message) {
            P2SC_Msg(LVL_FATAL_FILESYSTEM, "%s", err->message);
            g_error_free(err);
        } else
            P2SC_Msg(LVL_FATAL_FILESYSTEM, "%s: file reading failed: unknown reason", name);
    }

    return map;
}

static GArray *far = NULL;

static int descend(const char *name, const struct stat *st G_GNUC_UNUSED,
                   int fl, struct FTW *ft G_GNUC_UNUSED) {
    if (fl == FTW_F) {
        char *n = g_strdup(name);
        g_array_append_val(far, n);
    }

    return 0;
}

char **p2sc_dirscan(const char *name) {
    gboolean segfree = TRUE;
    char **ret = NULL;

    if (far)
        P2SC_Msg(LVL_FATAL_INTERNAL_ERROR, "far != NULL");
    else
        far = g_array_new(TRUE, FALSE, sizeof(char *));

    if (nftw(name, descend, 64, 0) == -1)
        P2SC_Msg(LVL_FATAL_FILESYSTEM, "nftw(%s): %s", name, g_strerror(errno));
    else
        segfree = FALSE;

    ret = (char **) g_array_free(far, segfree);
    far = NULL;

    return ret;
}
