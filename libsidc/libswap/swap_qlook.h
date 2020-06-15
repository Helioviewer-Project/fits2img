/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

#ifndef __SWAP_QLOOK_H__
#define __SWAP_QLOOK_H__

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------- */

    void swap_denoise(float *, size_t, size_t, int, double);
    void swap_crispen(float *, size_t, size_t);

    void swap_diff(float *, size_t, size_t, const float *, size_t, size_t, double);

    void swap_clamp(float *, size_t, size_t, float, float);
    guint8 *swap_xfer_gamma(const float *, size_t, size_t, float, float, double);
    guint8 *swap_xfer_log(const float *, size_t, size_t, float, float);

/* ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif
#endif
