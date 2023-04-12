/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

#ifndef __P2SC_STDLIB_H__
#define __P2SC_STDLIB_H__

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------- */

#define PPT_LIBNAME "libppt"

#define P_GUINT64_FHEX "%016"G_GINT64_MODIFIER"x"
#define P_OBET_FHEX    "%012"G_GINT64_MODIFIER"x"
#define P_OBET_F       "%014"G_GINT64_MODIFIER"u"

    void p2sc_init(const char *, const char *, const char *, const char *);

    /*
       generic
       "filename"
       "prgname"
       "appname"
       "runid"
       "history"

       SW-TMR
       "jpeg_info"
     */
    void p2sc_set_string(const char *, const char *);
    const char *p2sc_get_string(const char *);

    void p2sc_option_ext(int, int *, char ***, const char *, const char *,
                         const char *, const GOptionEntry *);
    void p2sc_option(int, char **, const char *, const char *, const char *, const GOptionEntry *);

    int p2sc_spawn(const char *, char **, char **);
    void p2sc_spawn_many(const char **, int);
    int p2sc_spawn_lmat(const char *, const char *, char **, char **);

    void *p2sc_get_symbol(const char *, const char **, void **);

    int p2sc_string_isnumeric(const char *);

    char *p2sc_bin2hex(const guint8 *, size_t);
    guint8 *p2sc_hex2bin(const char *, size_t);

/* ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif
#endif
