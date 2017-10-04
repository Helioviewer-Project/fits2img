/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

#ifndef __FITSPROC_H__
#define __FITSPROC_H__

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------- */

    typedef struct {
        char *name;

        char *dateobs;
        char *telescop;
        char *instrume;
        char *detector;
        char *wavelnth;

        float *im;
        size_t w;
        size_t h;
        int swap;

        char *xml;
    } procfits_t;

    procfits_t *fitsproc(const char *, int);
    void procfits_free(procfits_t *);

/* ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif
#endif
