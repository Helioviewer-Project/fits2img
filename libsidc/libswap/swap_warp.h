/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

#ifndef __SWAP_WARP_H__
#define __SWAP_WARP_H__

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------- */

    float *swap_affine(swap_bicubic_t *, const float *, size_t, size_t,
                       double, double, double, double, double, double, size_t, size_t);

    float *swap_polar(const float *, size_t, size_t, size_t, size_t, double, double);

    float *swap_polar2(const float *, size_t, size_t, size_t, size_t, double, double, double);

    float *swap_rebin(const float *, size_t, size_t, size_t, size_t);

/* ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif
#endif
