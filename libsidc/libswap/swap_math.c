/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

static const char _versionid_[] __attribute__ ((unused)) =
    "$Id: swap_math.c 5113 2014-06-19 15:07:34Z bogdan $";

#include <string.h>
#include <float.h>
#include <math.h>
#include <glib.h>

#include "swap_math.h"
#include "swap_vliet.h"

static inline float fetch(const float *in, int w, int h, int i, int j)
{
    if (i < 0 || i > w - 1 || j < 0 || j > h - 1)
        return 0;
    else
        return in[j * w + i];
}

static inline void fetch16(const float *in, int w, int h, int i, int j,
                           float a[16])
{
    int im1 = i - 1, ip1 = i + 1, ip2 = i + 2;
    int jm1 = j - 1, jp1 = j + 1, jp2 = j + 2;

    if (i > 0 && i < w - 2 && j > 0 && j < h - 2) {
        in += jm1 * w + im1;
        memcpy(a, in, 4 * sizeof *in), in += w, a += 4;
        memcpy(a, in, 4 * sizeof *in), in += w, a += 4;
        memcpy(a, in, 4 * sizeof *in), in += w, a += 4;
        memcpy(a, in, 4 * sizeof *in);
    } else {
        a[0] = fetch(in, w, h, im1, jm1);
        a[1] = fetch(in, w, h, i, jm1);
        a[2] = fetch(in, w, h, ip1, jm1);
        a[3] = fetch(in, w, h, ip2, jm1);

        a[4] = fetch(in, w, h, im1, j);
        a[5] = fetch(in, w, h, i, j);
        a[6] = fetch(in, w, h, ip1, j);
        a[7] = fetch(in, w, h, ip2, j);

        a[8] = fetch(in, w, h, im1, jp1);
        a[9] = fetch(in, w, h, i, jp1);
        a[10] = fetch(in, w, h, ip1, jp1);
        a[11] = fetch(in, w, h, ip2, jp1);

        a[12] = fetch(in, w, h, im1, jp2);
        a[13] = fetch(in, w, h, i, jp2);
        a[14] = fetch(in, w, h, ip1, jp2);
        a[15] = fetch(in, w, h, ip2, jp2);
    }
}

float swap_fetch9(const float *in, int w, int h, int i, int j, float a[9])
{
    int im1 = i - 1, ip1 = i + 1;
    int jm1 = j - 1, jp1 = j + 1;

    if (i > 0 && i < w - 1 && j > 0 && j < h - 1) {
        in += jm1 * w + im1;
        memcpy(a + 0, in, 3 * sizeof *in), in += w;
        memcpy(a + 3, in, 3 * sizeof *in), in += w;
        memcpy(a + 6, in, 3 * sizeof *in);
    } else {
        a[0] = fetch(in, w, h, im1, jm1);
        a[1] = fetch(in, w, h, i, jm1);
        a[2] = fetch(in, w, h, ip1, jm1);

        a[3] = fetch(in, w, h, im1, j);
        a[4] = fetch(in, w, h, i, j);
        a[5] = fetch(in, w, h, ip1, j);

        a[6] = fetch(in, w, h, im1, jp1);
        a[7] = fetch(in, w, h, i, jp1);
        a[8] = fetch(in, w, h, ip1, jp1);
    }

    return a[4];
}

#define NC 8192

struct swap_bicubic_t {
    float l[4 * NC];
    float n[16];
    int idx;
};

/* (8) http://userweb.cs.utexas.edu/users/fussell/courses/cs384g/lectures/mitchell/Mitchell.pdf */
static inline float cc(float x, float Q[4])
{
    float xx = x * x;
    return Q[0] + Q[1] * x + Q[2] * xx + Q[3] * xx * x;
}

swap_bicubic_t *swap_bicubic_alloc(double B, double C)
{
    float Q0[4], Q1[4], s;
    swap_bicubic_t *f = (swap_bicubic_t *) g_malloc(sizeof *f);
    float *l = f->l;

    f->idx = 0xffffffff;

    Q0[0] = (6 - 2 * B) / 6;
    Q0[1] = 0;
    Q0[2] = (-18 + 12 * B + 6 * C) / 6;
    Q0[3] = (12 - 9 * B - 6 * C) / 6;

    Q1[0] = (8 * B + 24 * C) / 6;
    Q1[1] = (-12 * B - 48 * C) / 6;
    Q1[2] = (6 * B + 30 * C) / 6;
    Q1[3] = (-B - 6 * C) / 6;

    /* cache filter coeffs on a fine grid */
    s = Q1[0] + Q1[1] + Q1[2] + Q1[3];
    l[0] = s;
    l[1] = Q0[0];
    l[2] = s;
    l[3] = 0;

    for (int i = 1; i < NC; ++i) {
        float d = i / (float) NC;

        l += 4;
        l[0] = cc(d + 1, Q1);
        l[1] = cc(d, Q0);
        l[2] = cc(1 - d, Q0);
        l[3] = cc(2 - d, Q1);
    }

    return f;
}

void swap_bicubic_free(swap_bicubic_t * f)
{
    if (f) {
        memset(f, 0, sizeof *f);
        g_free(f);
    }
}

static inline int _floor(float f)
{
    if (f >= 0)
        return f;
    else
        return floorf(f);
}

float swap_bicubic(swap_bicubic_t * f, const float *in, int w, int h, float x,
                   float y)
{
    float *hf, *vf, a0, a1, a2, a3;
    int i = (int) _floor(x), j = (int) _floor(y), idx = (j << 16) + i;
    float *l = f->l, *n = f->n;

    /* avoid neighborhood re-fetch */
    if (idx != f->idx) {
        fetch16(in, w, h, i, j, n);
        f->idx = idx;
    }

    /* approximate displacement coeffs by pre-computed grid */
    hf = l + 4 * (int) ((x - i) * NC);
    a0 = n[0] * hf[0] + n[1] * hf[1] + n[2] * hf[2] + n[3] * hf[3];
    a1 = n[4] * hf[0] + n[5] * hf[1] + n[6] * hf[2] + n[7] * hf[3];
    a2 = n[8] * hf[0] + n[9] * hf[1] + n[10] * hf[2] + n[11] * hf[3];
    a3 = n[12] * hf[0] + n[13] * hf[1] + n[14] * hf[2] + n[15] * hf[3];

    vf = l + 4 * (int) ((y - j) * NC);
    return a0 * vf[0] + a1 * vf[1] + a2 * vf[2] + a3 * vf[3];
}

#define elem_type float

/*
 *  This Quickselect routine is based on the algorithm described in
 *  "Numerical recipes in C", Second Edition,
 *  Cambridge University Press, 1992, Section 8.5, ISBN 0-521-43108-5
 *  This code by Nicolas Devillard - 1998. Public domain.
 */

#define ELEM_SWAP(a,b) { register elem_type t=(a);(a)=(b);(b)=t; }
#define PIX_SORT(a,b) { if ((a)>(b)) ELEM_SWAP((a),(b)); }

elem_type swap_median(elem_type * arr, int n)
{

    /*
     * The following routines have been built from knowledge gathered
     * around the Web. I am not aware of any copyright problem with
     * them, so use it as you want.
     * N. Devillard - 1998
     */
    switch (n) {
    case 3:
        /* found on sci.image.processing */
        {
            PIX_SORT(arr[0], arr[1]);
            PIX_SORT(arr[1], arr[2]);
            PIX_SORT(arr[0], arr[1]);
            return arr[1];
        }
    case 5:
        /* found on sci.image.processing */
        {
            PIX_SORT(arr[0], arr[1]);
            PIX_SORT(arr[3], arr[4]);
            PIX_SORT(arr[0], arr[3]);
            PIX_SORT(arr[1], arr[4]);
            PIX_SORT(arr[1], arr[2]);
            PIX_SORT(arr[2], arr[3]);
            PIX_SORT(arr[1], arr[2]);
            return arr[2];
        }
    case 7:
        /* found on sci.image.processing */
        {
            PIX_SORT(arr[0], arr[5]);
            PIX_SORT(arr[0], arr[3]);
            PIX_SORT(arr[1], arr[6]);
            PIX_SORT(arr[2], arr[4]);
            PIX_SORT(arr[0], arr[1]);
            PIX_SORT(arr[3], arr[5]);
            PIX_SORT(arr[2], arr[6]);
            PIX_SORT(arr[2], arr[3]);
            PIX_SORT(arr[3], arr[6]);
            PIX_SORT(arr[4], arr[5]);
            PIX_SORT(arr[1], arr[4]);
            PIX_SORT(arr[1], arr[3]);
            PIX_SORT(arr[3], arr[4]);
            return arr[3];
        }
    case 9:
        /* Formula from: XILINX XCELL magazine, vol. 23 by John L. Smith
           The input array is modified in the process.
           The result array is guaranteed to contain the median value
           in middle position, but other elements are NOT sorted. */
        {
            PIX_SORT(arr[1], arr[2]);
            PIX_SORT(arr[4], arr[5]);
            PIX_SORT(arr[7], arr[8]);
            PIX_SORT(arr[0], arr[1]);
            PIX_SORT(arr[3], arr[4]);
            PIX_SORT(arr[6], arr[7]);
            PIX_SORT(arr[1], arr[2]);
            PIX_SORT(arr[4], arr[5]);
            PIX_SORT(arr[7], arr[8]);
            PIX_SORT(arr[0], arr[3]);
            PIX_SORT(arr[5], arr[8]);
            PIX_SORT(arr[4], arr[7]);
            PIX_SORT(arr[3], arr[6]);
            PIX_SORT(arr[1], arr[4]);
            PIX_SORT(arr[2], arr[5]);
            PIX_SORT(arr[4], arr[7]);
            PIX_SORT(arr[4], arr[2]);
            PIX_SORT(arr[6], arr[4]);
            PIX_SORT(arr[4], arr[2]);
            return arr[4];
        }
    case 25:
        /* Code taken from Graphic Gems. */
        {
            PIX_SORT(arr[0], arr[1]);
            PIX_SORT(arr[3], arr[4]);
            PIX_SORT(arr[2], arr[4]);
            PIX_SORT(arr[2], arr[3]);
            PIX_SORT(arr[6], arr[7]);
            PIX_SORT(arr[5], arr[7]);
            PIX_SORT(arr[5], arr[6]);
            PIX_SORT(arr[9], arr[10]);
            PIX_SORT(arr[8], arr[10]);
            PIX_SORT(arr[8], arr[9]);
            PIX_SORT(arr[12], arr[13]);
            PIX_SORT(arr[11], arr[13]);
            PIX_SORT(arr[11], arr[12]);
            PIX_SORT(arr[15], arr[16]);
            PIX_SORT(arr[14], arr[16]);
            PIX_SORT(arr[14], arr[15]);
            PIX_SORT(arr[18], arr[19]);
            PIX_SORT(arr[17], arr[19]);
            PIX_SORT(arr[17], arr[18]);
            PIX_SORT(arr[21], arr[22]);
            PIX_SORT(arr[20], arr[22]);
            PIX_SORT(arr[20], arr[21]);
            PIX_SORT(arr[23], arr[24]);
            PIX_SORT(arr[2], arr[5]);
            PIX_SORT(arr[3], arr[6]);
            PIX_SORT(arr[0], arr[6]);
            PIX_SORT(arr[0], arr[3]);
            PIX_SORT(arr[4], arr[7]);
            PIX_SORT(arr[1], arr[7]);
            PIX_SORT(arr[1], arr[4]);
            PIX_SORT(arr[11], arr[14]);
            PIX_SORT(arr[8], arr[14]);
            PIX_SORT(arr[8], arr[11]);
            PIX_SORT(arr[12], arr[15]);
            PIX_SORT(arr[9], arr[15]);
            PIX_SORT(arr[9], arr[12]);
            PIX_SORT(arr[13], arr[16]);
            PIX_SORT(arr[10], arr[16]);
            PIX_SORT(arr[10], arr[13]);
            PIX_SORT(arr[20], arr[23]);
            PIX_SORT(arr[17], arr[23]);
            PIX_SORT(arr[17], arr[20]);
            PIX_SORT(arr[21], arr[24]);
            PIX_SORT(arr[18], arr[24]);
            PIX_SORT(arr[18], arr[21]);
            PIX_SORT(arr[19], arr[22]);
            PIX_SORT(arr[8], arr[17]);
            PIX_SORT(arr[9], arr[18]);
            PIX_SORT(arr[0], arr[18]);
            PIX_SORT(arr[0], arr[9]);
            PIX_SORT(arr[10], arr[19]);
            PIX_SORT(arr[1], arr[19]);
            PIX_SORT(arr[1], arr[10]);
            PIX_SORT(arr[11], arr[20]);
            PIX_SORT(arr[2], arr[20]);
            PIX_SORT(arr[2], arr[11]);
            PIX_SORT(arr[12], arr[21]);
            PIX_SORT(arr[3], arr[21]);
            PIX_SORT(arr[3], arr[12]);
            PIX_SORT(arr[13], arr[22]);
            PIX_SORT(arr[4], arr[22]);
            PIX_SORT(arr[4], arr[13]);
            PIX_SORT(arr[14], arr[23]);
            PIX_SORT(arr[5], arr[23]);
            PIX_SORT(arr[5], arr[14]);
            PIX_SORT(arr[15], arr[24]);
            PIX_SORT(arr[6], arr[24]);
            PIX_SORT(arr[6], arr[15]);
            PIX_SORT(arr[7], arr[16]);
            PIX_SORT(arr[7], arr[19]);
            PIX_SORT(arr[13], arr[21]);
            PIX_SORT(arr[15], arr[23]);
            PIX_SORT(arr[7], arr[13]);
            PIX_SORT(arr[7], arr[15]);
            PIX_SORT(arr[1], arr[9]);
            PIX_SORT(arr[3], arr[11]);
            PIX_SORT(arr[5], arr[17]);
            PIX_SORT(arr[11], arr[17]);
            PIX_SORT(arr[9], arr[17]);
            PIX_SORT(arr[4], arr[10]);
            PIX_SORT(arr[6], arr[12]);
            PIX_SORT(arr[7], arr[14]);
            PIX_SORT(arr[4], arr[6]);
            PIX_SORT(arr[4], arr[7]);
            PIX_SORT(arr[12], arr[14]);
            PIX_SORT(arr[10], arr[14]);
            PIX_SORT(arr[6], arr[7]);
            PIX_SORT(arr[10], arr[12]);
            PIX_SORT(arr[6], arr[10]);
            PIX_SORT(arr[6], arr[17]);
            PIX_SORT(arr[12], arr[17]);
            PIX_SORT(arr[7], arr[17]);
            PIX_SORT(arr[7], arr[10]);
            PIX_SORT(arr[12], arr[18]);
            PIX_SORT(arr[7], arr[12]);
            PIX_SORT(arr[10], arr[18]);
            PIX_SORT(arr[12], arr[20]);
            PIX_SORT(arr[10], arr[20]);
            PIX_SORT(arr[10], arr[12]);
            return arr[12];
        }
    }

    int low = 0, high = n - 1;
    int median, middle, ll, hh;

    median = (low + high) / 2;
    for (;;) {
        if (high <= low)        /* One element only */
            return arr[median];

        if (high == low + 1) {  /* Two elements only */
            if (arr[low] > arr[high])
                ELEM_SWAP(arr[low], arr[high]);
            return arr[median];
        }

        /* Find median of low, middle and high items; swap into position low */
        middle = (low + high) / 2;
        if (arr[middle] > arr[high])
            ELEM_SWAP(arr[middle], arr[high]);
        if (arr[low] > arr[high])
            ELEM_SWAP(arr[low], arr[high]);
        if (arr[middle] > arr[low])
            ELEM_SWAP(arr[middle], arr[low]);

        /* Swap low item (now in position middle) into position (low+1) */
        ELEM_SWAP(arr[middle], arr[low + 1]);

        /* Nibble from each end towards middle, swapping items when stuck */
        ll = low + 1;
        hh = high;
        for (;;) {
            do
                ++ll;
            while (arr[low] > arr[ll]);
            do
                --hh;
            while (arr[hh] > arr[low]);

            if (hh < ll)
                break;

            ELEM_SWAP(arr[ll], arr[hh]);
        }

        /* Swap middle item (in position low) back into correct position */
        ELEM_SWAP(arr[low], arr[hh]);

        /* Re-set active partition */
        if (hh <= median)
            low = ll;
        if (hh >= median)
            high = hh - 1;
    }
}

float *swap_dog(const float *in, size_t w, size_t h, double s1, double s2)
{
    size_t i, l = w * h;
    float *b1 = (float *) g_malloc(l * sizeof *b1);
    float *b2 = (float *) g_malloc(l * sizeof *b2);

    swap_gauss(in, b1, w, h, s1);
    swap_gauss(in, b2, w, h, s2);
    for (i = 0; i < l; ++i)
        b1[i] -= b2[i];

    g_free(b2);

    return b1;
}

double swap_mse(const float *im1, const float *im2, size_t w, size_t h,
                size_t X1, size_t Y1, size_t X2, size_t Y2)
{
    double d, mse = 0;
    size_t i, j, n = 0;

    X1 = MIN(X1, w), Y1 = MIN(Y1, h), X2 = MIN(X2, w), Y2 = MIN(Y2, h);
    for (j = Y1; j < Y2; ++j)
        for (i = X1; i < X2; ++i) {
            ++n;
            d = im1[j * w + i] - im2[j * w + i];
            mse += (d * d - mse) / n;
        }

    return mse;
}

#define NH 32768
#define HC(x)   ((size_t) ((NH - 1) * (x - min) / (max - min) + .5))

static void top_quant(float *in, size_t len, float min, float max, double quant)
{
    size_t i, q = 0, qi = (1 - quant) * len + .5;
    size_t *hist = (size_t *) g_malloc0(NH * sizeof *hist);

    for (i = 0; i < len; ++i)
        ++hist[HC(in[i])];
    for (i = 0; i < NH; ++i) {
        q += hist[i];
        if (q > qi)
            break;
    }

    min += i * (max - min) / (NH - 1);
    for (i = 0; i < len; ++i) {
        in[i] = (in[i] - min) * 16;
        if (in[i] < 0)
            in[i] = 0;
    }

    g_free(hist);

}

#define SF(k,l) fetch(in, w, h, i + (k), j + (l))

float *swap_madmax(const float *in, size_t w, size_t h)
{
    size_t i, j, l = w * h;
    float h1 = 0.5, h2 = 0.2 * sqrt(5), h3 = 0.25 * sqrt(2), m;
    float min = FLT_MAX, max = FLT_MIN;
    float *o = (float *) g_malloc(l * sizeof *o);

    for (j = 0; j < h; ++j)
        for (i = 0; i < w; ++i) {
            float p = fetch(in, w, h, i, j);
            float d0 = h1 * (p - .5 * (SF(+0, -2) + SF(+0, +2)));
            float d1 = h2 * (p - .5 * (SF(-1, -2) + SF(+1, +2)));
            float d2 = h3 * (p - .5 * (SF(-2, -2) + SF(+2, +2)));
            float d3 = h2 * (p - .5 * (SF(-2, -1) + SF(+2, +1)));
            float d4 = h1 * (p - .5 * (SF(-2, +0) + SF(+2, +0)));
            float d5 = h2 * (p - .5 * (SF(-2, +1) + SF(+2, -1)));
            float d6 = h3 * (p - .5 * (SF(-2, +2) + SF(+2, -2)));
            float d7 = h2 * (p - .5 * (SF(-1, +2) + SF(+1, -2)));

            m = MAX(d0, d1);
            m = MAX(m, d2);
            m = MAX(m, d3);
            m = MAX(m, d4);
            m = MAX(m, d5);
            m = MAX(m, d6);
            m = MAX(m, d7);

            o[j * w + i] = m;

            if (m < min)
                min = m;
            if (m > max)
                max = m;
        }

/*    for (i = 0; i < l; ++i)
        o[i] -= min;
*/ top_quant(o, l, min, max, 0.98);

    return o;
}

#define BARY_TRESH .66

void swap_bary(const float *in, size_t w, size_t h, float *xc, float *yc)
{
    size_t i, j;
    double s = 0, v;

    float *c = (float *) g_malloc0(w * sizeof *c);
    float *r = (float *) g_malloc0(h * sizeof *r);

    for (j = 0; j < h; ++j)
        for (i = 0; i < w; ++i) {
            v = in[j * w + i];
            c[i] += v;
            r[j] += v;
            s += v;
        }

    v = 0;
    double sw = (BARY_TRESH / w) * s, sc = 0, cc;
    for (i = 0; i < w; ++i) {
        cc = c[i];
        if (cc > sw)
            cc = sw;
        v += cc * (i + .5);
        sc += cc;
    }
    *xc = v / sc;

    v = 0;
    double sh = (BARY_TRESH / h) * s, sr = 0, rr;
    for (j = 0; j < h; ++j) {
        rr = r[j];
        if (rr > sh)
            rr = sh;
        v += rr * (j + .5);
        sr += rr;
    }
    *yc = v / sr;

    g_free(c), g_free(r);
}
