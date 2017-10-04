/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

static const char _versionid_[] __attribute__ ((unused)) =
    "$Id: swap_qlook.c 5110 2014-06-19 12:37:15Z bogdan $";

#include <math.h>
#include <string.h>
#include <glib.h>

#include "swap_math.h"
#include "swap_qlook.h"
#include "swap_vliet.h"

void swap_denoise(float *out, size_t w, size_t h, int dc, double ns)
{
    size_t i, j;
    float d[9], a[9], t, p, med, mad;
    float *in = (float *) g_malloc(w * h * sizeof *in);

    memcpy(in, out, w * h * sizeof *in);

    t = 0;
    if (dc) {
        /* dark level from 9 patches */
        swap_fetch9(in, w, h, 1, 1, a);
        d[0] = swap_median(a, 9);
        swap_fetch9(in, w, h, w / 2, 1, a);
        d[1] = swap_median(a, 9);
        swap_fetch9(in, w, h, w - 2, 1, a);
        d[2] = swap_median(a, 9);

        swap_fetch9(in, w, h, 1, h / 2, a);
        d[3] = swap_median(a, 9);
        swap_fetch9(in, w, h, w / 2, h / 2, a);
        d[4] = swap_median(a, 9);
        swap_fetch9(in, w, h, w - 2, h / 2, a);
        d[5] = swap_median(a, 9);

        swap_fetch9(in, w, h, 1, h - 2, a);
        d[6] = swap_median(a, 9);
        swap_fetch9(in, w, h, w / 2, h - 2, a);
        d[7] = swap_median(a, 9);
        swap_fetch9(in, w, h, w - 2, h - 2, a);
        d[8] = swap_median(a, 9);

        t = swap_median(d, 9);
        t += 3 * sqrt(t);
    }

    for (j = 0; j < h; ++j)
        for (i = 0; i < w; ++i) {
            p = swap_fetch9(in, w, h, i, j, a);
            med = swap_median(a, 9);

            /* presumably just dark noise */
            if (p < t) {
                *out++ = med;
                continue;
            }

            a[0] = fabsf(a[0] - med);
            a[1] = fabsf(a[1] - med);
            a[2] = fabsf(a[2] - med);
            a[3] = fabsf(a[3] - med);
            a[4] = fabsf(a[4] - med);
            a[5] = fabsf(a[5] - med);
            a[6] = fabsf(a[6] - med);
            a[7] = fabsf(a[7] - med);
            a[8] = fabsf(a[8] - med);
            mad = swap_median(a, 9);

            /* outlier */
            if (fabsf(p - med) > ns * 1.4826 * mad)
                *out++ = med;
            else
                *out++ = p;
        }
    g_free(in);
}

#define SIGMA  1/1.6
#define NARROW 1
#define WIDE   .5
#define BLUR   1

void swap_crispen(float *out, size_t w, size_t h)
{
    size_t i, l = w * h;
    float *in = (float *) g_malloc(3 * l * sizeof *in);

    /* apply first a slight blur to reduce pixel scale artifacts */
    swap_gauss(out, in + 0 * l, w, h, .5);
    for (i = 0; i < l; ++i)
        out[i] += BLUR * (in[i + 0 * l] - out[i]);

    swap_gauss(out, in + 0 * l, w, h, SIGMA);
    swap_gauss(out, in + 1 * l, w, h, SIGMA * 1.6);
    swap_gauss(out, in + 2 * l, w, h, SIGMA * 3.2);

    /* DoG narrow & wide */
    for (i = 0; i < l; ++i)
        out[i] += (NARROW + WIDE) * in[i + 0 * l]
            - NARROW * in[i + l] - WIDE * in[i + 2 * l];

    g_free(in);
}

void swap_diff(float *im2, size_t w2, size_t h2, const float *im1, size_t w1,
               size_t h1, double th)
{
    size_t W = MIN(w2, w1), H = MIN(h2, h1), i, j;

    for (j = 0; j < H; ++j)
        for (i = 0; i < W; ++i) {
            float d = im2[j * w2 + i] - im1[j * w1 + i];

            if (d < -th)
                d = -th;
            else if (d > th)
                d = th;
            im2[j * w2 + i] = (d / th + 1) * (4095. / 2);
        }

    for (j = H; j < h2; ++j)
        for (i = W; i < w2; ++i)
            im2[j * w2 + i] = 2047;
}

guint8 *swap_xfer_gamma(const float *in, size_t w, size_t h, float lo, float hi,
                        double g)
{
    size_t len = w * h, i;
    guint8 *out = (guint8 *) g_malloc(len * sizeof *out), *o = out;

    if (hi == -1000000)
        for (i = 0; i < len; ++i)
            if (in[i] > hi)
                hi = in[i];

    double r = hi - lo;
    if (r <= 0) {
        memset(out, 0, len * sizeof *out);
        return out;
    }

    double g1 = 1. / g, pr = 255. / pow(r, g1);
    for (i = 0; i < len; ++i) {
        double p = *in++ - lo;
        p = pow(CLAMP(p, 0, r), g1) * pr + .5;
        *o++ = CLAMP(p, 0, 255);
    }

    return out;
}

guint8 *swap_xfer_log(const float *in, size_t w, size_t h, float lo, float hi)
{
    size_t len = w * h, i;
    guint8 *out = (guint8 *) g_malloc(len * sizeof *out), *o = out;

    if (hi == -1000000)
        for (i = 0; i < len; ++i)
            if (in[i] > hi)
                hi = in[i];

    double r = hi - lo;
    if (r <= 0) {
        memset(out, 0, len * sizeof *out);
        return out;
    }

    double lr = 255 / log1p(r);
    for (i = 0; i < len; ++i) {
        double p = *in++ - lo;
        p = log1p(CLAMP(p, 0, r)) * lr + .5;
        *o++ = CLAMP(p, 0, 255);
    }

    return out;
}
