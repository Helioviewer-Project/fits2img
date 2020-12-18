/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

#ifndef __SWAP_FILE_J2K_H__
#define __SWAP_FILE_J2K_H__

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------- */

    typedef struct {
        double cratio;
        int nlayers;
        int nresolutions;
        int precinct[2];
        /* client data - should stay in sync with opj_extra.c */
        struct {
            const char *xml;
            const unsigned char (*pal)[256][3];
        } meta;
        int debug;
    } swap_j2kparams_t;

    void swap_write_j2k(const char *, const guint8 *, size_t, size_t, const swap_j2kparams_t *);

    guint8 *swap_read_j2k(const char *name, size_t *, size_t *, size_t *);

/* ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif
#endif
