/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

#ifndef __P2SC_NAME_H__
#define __P2SC_NAME_H__

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------- */

    char *p2sc_name_dirtree(const char *, const char *);

    char *p2sc_name_swap_tmr(const char *, const guint8 *, size_t, guint64);
    char *p2sc_name_swap_lv0(const char *, const char *);

    char *p2sc_name_swap_qlk(const char *, const char *, const char *);
    char *p2sc_name_swap_jhv(const char *, const char *, const char *, const char *, const char *);

/* ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif
#endif
