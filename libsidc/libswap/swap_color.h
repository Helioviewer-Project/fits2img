/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

#ifndef __SWAP_COLOR_H__
#define __SWAP_COLOR_H__

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------- */

    typedef struct {
        unsigned char *y;
        unsigned char *u;
        unsigned char *v;
        size_t w, h;
    } swap_image_yuv_t;

    typedef const unsigned char swap_palette_t[256][3];
    swap_palette_t *swap_palette_rgb_get(const char *);

    swap_image_yuv_t *swap_image_yuv_alloc(size_t, size_t);
    void swap_image_yuv_free(swap_image_yuv_t *);

    swap_image_yuv_t *swap_mono2yuv(const char *, const guint8 *, size_t,
                                    size_t);
    void swap_yuv2yuv420(swap_image_yuv_t *);

/* ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif
#endif
