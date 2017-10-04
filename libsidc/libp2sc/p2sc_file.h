/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

#ifndef __P2SC_FILE_H__
#define __P2SC_FILE_H__

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------- */

#define P2SC_GLOBAL_INI "/p2sc/conf/global.ini"

    typedef struct p2sc_keyfile_t p2sc_keyfile_t;

    p2sc_keyfile_t *p2sc_open_keyfile(const char *);
    void p2sc_free_keyfile(p2sc_keyfile_t *);
    char *p2sc_get_keyvalue(p2sc_keyfile_t *, const char *, const char *);

    char *p2sc_get_temp(const char *);

    typedef struct p2sc_iofile_t p2sc_iofile_t;

    p2sc_iofile_t *p2sc_open_iofile(const char *, const char *);
    void p2sc_free_iofile(p2sc_iofile_t *);
    char *p2sc_read_line(p2sc_iofile_t *);
    size_t p2sc_get_lineno(p2sc_iofile_t *);

    void p2sc_write(p2sc_iofile_t *, const char *, gssize);
    void p2sc_flush(p2sc_iofile_t *);

    char *p2sc_test_file_input(const char *, const char *);
    char *p2sc_test_file_output(const char *, const char *);

    char **p2sc_readlines_file(const char *, int);

    void p2sc_strip_strings(char ***);

    int p2sc_create_file(int, const char *, void *, size_t);

    void p2sc_copy_file(const char *, const char *);

    GMappedFile *p2sc_map_file(const char *);

    char **p2sc_dirscan(const char *);

/* ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif
#endif
