/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

#ifndef __SWAP_COORD_H__
#define __SWAP_COORD_H__

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------- */

#define SWAP_NP      1024

/* arcsecs */
#define SWAP_CDELT1 3.1646941
#define SWAP_CDELT2 3.1760467

/* arcdegrees */
#define SWAP_FOVX   (SWAP_CDELT1 * SWAP_NP / 3600.)
#define SWAP_FOVY   (SWAP_CDELT2 * SWAP_NP / 3600.)

/* pixels <-> radians, LV1 FOV = SWAP_FOVX */
#define SWAP_P2R(ss) ((SWAP_FOVX * M_PI / 180.) / (SWAP_NP * ss))
#define SWAP_R2P(ss) ((SWAP_NP * ss) / (SWAP_FOVX * M_PI / 180.))

    void swap_pix2vec(int, double, double, double v[3]);
    void swap_vec2pix(int, double v[3], double *, double *);

    typedef struct swap_pix2vec_lut_t swap_pix2vec_lut_t;

    swap_pix2vec_lut_t *swap_pix2vec_lut_alloc(size_t);
    void swap_pix2vec_lut_free(swap_pix2vec_lut_t *);

    double *swap_vbore(swap_pix2vec_lut_t *);

/* ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif
#endif
