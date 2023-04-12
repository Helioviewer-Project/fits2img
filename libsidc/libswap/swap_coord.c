/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

static const char _versionid_[] __attribute__((unused)) =
    "$Id: swap_coord.c 5113 2014-06-19 15:07:34Z bogdan $";

#include <math.h>
#include <string.h>
#include <glib.h>

#include "swap_coord.h"

#define SWAP_NP2(ss) (.5 * ((SWAP_NP * ss) - 1))

/* P2SEQ <-> helioprojective-cartesian <-> image pixels */
/* ref. "Coordinate systems for solar image data", W. T. Thompson, A&A 2005 */

/* C image convention */
/* use pseudo angles approximation, instead of (26) */
/* right ascension should be atan(X) */
/* declination should be atan2(Y, hypot(1, X)) */

static inline void pix2ra(int ss, double x, double ra[2]) {
    double X = (+x - SWAP_NP2(ss)) * SWAP_P2R(ss);

    ra[0] = sin(X);
    ra[1] = cos(X);
}

static inline void pix2de(int ss, double y, double de[2]) {
    double Y = (-y + SWAP_NP2(ss)) * SWAP_P2R(ss);

    de[0] = sin(Y);
    de[1] = cos(Y);
}

void swap_pix2vec(int ss, double x, double y, double v[3]) {
    double ra[2], de[2];

    pix2ra(ss, x, ra);
    pix2de(ss, y, de);
    /* radrec, Z mirrored */
    v[0] = +de[1] * ra[0];      /* +cos(de) * sin(ra) */
    v[1] = +de[0];              /* +sin(de)           */
    v[2] = -de[1] * ra[1];      /* -cos(de) * cos(ra) */
}

static inline void rade2vec4(const double ra[2], const double de[2],
                             double v0[3], double v1[3], double v2[3], double v3[3]) {
    /* radrec, Z mirrored */
    double x = +de[1] * ra[0];  /* +cos(de) * sin(ra) */
    double y = -de[0];          /* +sin(de)   !!!!!   <- -de[0] from LUT */
    double z = -de[1] * ra[1];  /* -cos(de) * cos(ra) */

    /* ul */
    v0[0] = x;
    v0[1] = y;
    v0[2] = z;
    /* ur */
    v1[0] = -x;
    v1[1] = y;
    v1[2] = z;
    /* ll */
    v2[0] = x;
    v2[1] = -y;
    v2[2] = z;
    /* lr */
    v3[0] = -x;
    v3[1] = -y;
    v3[2] = z;
}

/* ---------------------------------------------------------------------- */

struct swap_pix2vec_lut_t {
    double (*l)[2];
    size_t s;
};

swap_pix2vec_lut_t *swap_pix2vec_lut_alloc(size_t ss) {
    swap_pix2vec_lut_t *lut = (swap_pix2vec_lut_t *) g_malloc(sizeof *lut);

    lut->l = (double (*)[2]) g_malloc((SWAP_NP * ss) / 2 * sizeof *(lut->l));
    for (size_t i = 0; i < (SWAP_NP * ss) / 2; ++i)
        pix2ra(ss, i, lut->l[i]);

    lut->s = ss;
    return lut;
}

void swap_pix2vec_lut_free(swap_pix2vec_lut_t *lut) {
    if (lut) {
        g_free(lut->l);
        memset(lut, 0, sizeof *lut);
        g_free(lut);
    }
}

#if defined(__APPLE__) && defined(__MACH__)

#include <dispatch/dispatch.h>

#define START_LOOP(loop_idx, loop_len) \
    do { \
        dispatch_queue_t queue = \
            dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0); \
        dispatch_apply(loop_len, queue, ^(size_t loop_idx)

#define END_LOOP );} while (0)

#else

#define START_LOOP(loop_idx, loop_len) \
    do { \
        for (size_t loop_idx = 0; loop_idx < loop_len; ++loop_idx)
#define END_LOOP } while (0)

#endif

double *swap_vbore(swap_pix2vec_lut_t *lut) {
    size_t w = SWAP_NP * lut->s;
    double (*l)[2] = lut->l;
    double *vbore = (double *) g_malloc(3 * w * w * sizeof *vbore);

    START_LOOP(j, w / 2) {
        double *vb1 = vbore + 3 * w * j;
        double *vb2 = vbore + 3 * w * (w - 1 - j);

        for (size_t i = 0; i < w / 2; ++i)
            rade2vec4(l[i], l[j],
                      vb1 + 3 * i, vb1 + 3 * (w - 1 - i), vb2 + 3 * i, vb2 + 3 * (w - 1 - i));
    }
    END_LOOP;

    return vbore;
}

/* ---------------------------------------------------------------------- */

void swap_vec2pix(int ss, double v[3], double *x, double *y) {
    /* Z mirrored, recrad */
    double ra = atan2(v[0], -v[2]);
    double de = atan2(v[1], hypot(v[0], v[2]));

    /* use pseudo angles approximation, instead of (25), C image convention */
    *x = SWAP_NP2(ss) + SWAP_R2P(ss) * ra;  /* should be tan(ra) */
    *y = SWAP_NP2(ss) - SWAP_R2P(ss) * de;  /* should be tan(de) / cos(ra) */
}
