/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

#ifndef __SWAP_MATH_H__
#define __SWAP_MATH_H__

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------- */

    float swap_fetch9(const float *, int, int, int, int, float a[9]);

    typedef struct swap_bicubic_t swap_bicubic_t;

    swap_bicubic_t *swap_bicubic_alloc(double, double);
    void swap_bicubic_free(swap_bicubic_t *);
    float swap_bicubic(swap_bicubic_t *, const float *, int, int, float, float);

    float swap_median(float *, int);

    float *swap_dog(const float *, size_t, size_t, double, double);
    float *swap_madmax(const float *, size_t, size_t);

    double swap_mse(const float *, const float *, size_t, size_t,
                    size_t, size_t, size_t, size_t);

    void swap_bary(const float *, size_t, size_t, float *, float *);

/* ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif
#endif
