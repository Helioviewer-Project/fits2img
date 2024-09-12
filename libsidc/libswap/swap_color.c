/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

static const char _versionid_[] __attribute__((unused)) = "$Id: swap_color.c 5113 2014-06-19 15:07:34Z bogdan $";

#include <string.h>
#include <glib.h>

#include "color/color_aia171.h"
#include "color/color_eui174.h"
#include "color/color_eui304.h"
#include "color/color_eui1216.h"
#include "color/color_citrus.h"
#include "color/color_gray.h"
#include "color/color_hot.h"
#include "color/color_jet.h"

#include "swap_color.h"

swap_palette_t *swap_palette_rgb_get(const char *cm) {
    if (cm) {
        if (!strcmp(cm, "aia171"))
            return &cm_aia171_rgb;
        else if (!strcmp(cm, "eui174"))
            return &cm_eui174_rgb;
        else if (!strcmp(cm, "eui304"))
            return &cm_eui304_rgb;
        else if (!strcmp(cm, "eui1216"))
            return &cm_eui1216_rgb;
        else if (!strcmp(cm, "citrus"))
            return &cm_citrus_rgb;
        else if (!strcmp(cm, "hot"))
            return &cm_hot_rgb;
        else if (!strcmp(cm, "jet"))
/*            for (j = 0; j < 256; ++j) {
                for (i = 0; i < 3; ++i)
                    cmjet_rgb[j][i] = 255 * cmjet[j][i] + .5;
                printf("\t{%d, %d, %d},\n", cmjet_rgb[j][0], cmjet_rgb[j][1], cmjet_rgb[j][2]);
            }
*/
            return &cm_jet_rgb;
    }
    return NULL;
}

swap_image_yuv_t *swap_image_yuv_alloc(size_t w, size_t h) {
    size_t l = w * h;
    swap_image_yuv_t *i = (swap_image_yuv_t *) g_malloc(sizeof *i);

    i->y = (unsigned char *) g_malloc(l);
    i->u = (unsigned char *) g_malloc(l);
    i->v = (unsigned char *) g_malloc(l);
    i->w = w, i->h = h;

    return i;
}

void swap_image_yuv_free(swap_image_yuv_t *i) {
    if (i) {
        g_free(i->v), g_free(i->u), g_free(i->y);
        memset(i, 0, sizeof *i);
        g_free(i);
    }
}

static void rgb2yuv(const double (*cmrgb)[3], unsigned char (*cmyuv)[3]) {
    const double wr = 0.299, wb = 0.114, umax = 0.436, vmax = 0.615;
    const double wg = 1 - wr - wb;

    for (int i = 0; i < 256; ++i) {
        double R = cmrgb[i][0], G = cmrgb[i][1], B = cmrgb[i][2];
        double y = wr * R + wg * G + wb * B;
        double u = (umax / (1 - wb)) * (B - y);
        double v = (vmax / (1 - wr)) * (R - y);

        y = CLAMP(y, 0, 1);
        u = CLAMP(u, -umax, umax);
        v = CLAMP(v, -vmax, vmax);

        cmyuv[i][0] = 255 * y + .5;
        cmyuv[i][1] = (255 / umax / 2) * (u + umax) + .5;
        cmyuv[i][2] = (255 / vmax / 2) * (v + vmax) + .5;
    }
}

static void cm_rgb2yuv(const char *cm, unsigned char (*cmyuv)[3]) {
    if (cm) {
        if (!strcmp(cm, "aia171")) {
            rgb2yuv(cm_aia171, cmyuv);
            return;
        } else if (!strcmp(cm, "eui174")) {
            rgb2yuv(cm_eui174, cmyuv);
            return;
        } else if (!strcmp(cm, "eui304")) {
            rgb2yuv(cm_eui304, cmyuv);
            return;
        } else if (!strcmp(cm, "eui1216")) {
            rgb2yuv(cm_eui1216, cmyuv);
            return;
        } else if (!strcmp(cm, "citrus")) {
            rgb2yuv(cm_citrus, cmyuv);
            return;
        } else if (!strcmp(cm, "hot")) {
            rgb2yuv(cm_hot, cmyuv);
            return;
        } else if (!strcmp(cm, "jet")) {
            rgb2yuv(cm_jet, cmyuv);
            return;
        }
    }
    /* gray */
    memcpy(cmyuv, cm_gray_yuv, 256 * 3);
}

swap_image_yuv_t *swap_mono2yuv(const char *cm, const guint8 *in, size_t w, size_t h) {
    size_t l = w * h;
    unsigned char lutyuv[256][3];
    cm_rgb2yuv(cm, lutyuv);

    swap_image_yuv_t *im = swap_image_yuv_alloc(w, h);
    unsigned char *y = im->y, *u = im->u, *v = im->v;

    while (l--) {
        const guint8 *yuv = lutyuv[*in++];
        *y++ = yuv[0];
        *u++ = yuv[1];
        *v++ = yuv[2];
    }

    return im;
}

void swap_yuv2yuv420(swap_image_yuv_t *im) {
    size_t w = im->w, h = im->h, i, j;
    unsigned char *u = im->u, *v = im->v;

    for (j = 0; j < h; j += 2)
        for (i = 0; i < w; i += 2) {
            int val;
            size_t idx = (j / 2) * (w / 2) + (i / 2);

            val = ((int) u[j * w + i] +
                   (int) u[j * w + (i + 1)] + (int) u[(j + 1) * w + i] + (int) u[(j + 1) * w + (i + 1)] + 2) >> 2;
            if (val > 255)
                val = 255;
            u[idx] = val;

            val = ((int) v[j * w + i] +
                   (int) v[j * w + (i + 1)] + (int) v[(j + 1) * w + i] + (int) v[(j + 1) * w + (i + 1)] + 2) >> 2;
            if (val > 255)
                val = 255;
            v[idx] = val;
        }
}
