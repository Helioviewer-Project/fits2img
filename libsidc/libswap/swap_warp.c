/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

static const char _versionid_[] __attribute__ ((unused)) =
    "$Id: swap_warp.c 5110 2014-06-19 12:37:15Z bogdan $";

#include <unistd.h>
#include <math.h>
#include <string.h>
#include <glib.h>

#include "p2sc_fits.h"
#include "p2sc_math.h"
#include "p2sc_msg.h"

#include "swap_math.h"
#include "swap_warp.h"

static inline void mxm(double m1[3][3], double m2[3][3], double mo[3][3])
{
    double mt[3][3];

    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            mt[i][j] =
                m1[i][0] * m2[0][j] + m1[i][1] * m2[1][j] + m1[i][2] * m2[2][j];
    memcpy(mo, mt, sizeof mt);
}

static void trns(double tx, double ty, double mo[3][3])
{
    double m[3][3] = { {1, 0, tx}, {0, 1, ty}, {0, 0, 1} };
    mxm(mo, m, mo);
}

static void scal(double sx, double sy, double mo[3][3])
{
    double m[3][3] = { {sx, 0, 0}, {0, sy, 0}, {0, 0, 1} };
    mxm(mo, m, mo);
}

static void rota(double a, double mo[3][3])
{
    double s, c;
    sincosd(a, &s, &c);
    double m[3][3] = { {c, s, 0}, {-s, c, 0}, {0, 0, 1} };

    mxm(mo, m, mo);
}

float *swap_affine(swap_bicubic_t * f, const float *in, size_t w, size_t h,
                   double sx, double sy, double roll, double tx, double ty,
                   double a, size_t ow, size_t oh)
{
    size_t i, j;
    float *out = (float *) g_malloc0(ow * oh * sizeof *out), *outi = out;

    double xc = (w - 1) / 2., yc = (h - 1) / 2.;
    double oxc = xc - ((double) w - (double) ow) / 2.;
    double oyc = yc - ((double) h - (double) oh) / 2.;

    const double iden[3][3] = { {1, 0, 0}, {0, 1, 0}, {0, 0, 1} };
    double m[3][3] = { {1, 0, 0}, {0, 1, 0}, {0, 0, 1} };

    /* mapping output to input: build it the reverse to avoid inverting */
    /* apply the inverse transforms + backmultiply the matrices */

    /* translate to input image center */
    trns(xc, yc, m);
    /* scale */
    scal(1 / sx, 1 / sy, m);
    /* periodic rolls */
    rota(-roll, m);
    /* translate */
    trns(-tx, -ty, m);
    /* ecliptic to solar axis */
    rota(-a, m);
    /* translate to (0,0) of output image */
    trns(-oxc, -oyc, m);

    for (j = 0; j < 3; ++j)
        for (i = 0; i < 3; ++i)
            if (!m[j][i])
                m[j][i] = 0;    /* -0 -> +0 */

    if (memcmp(m, iden, sizeof m)) {
        for (j = 0; j < oh; ++j)
            for (i = 0; i < ow; ++i)
                *outi++ = swap_bicubic(f, in, w, h,
                                       m[0][0] * i + m[0][1] * j + m[0][2],
                                       m[1][0] * i + m[1][1] * j + m[1][2]);
    } else {
        size_t row = MIN(w, ow) * sizeof *in;
        oh = MIN(h, oh);
        for (j = 0; j < oh; ++j)
            memcpy(out + j * ow, in + j * w, row);
    }

    return out;
}

#define AA 2.

static void polar_prep(guint32 * lut, size_t w, size_t h, guint16 * num,
                       int nang, int nrad, double xc, double yc)
{
    int ri, ai, k;
    size_t i, j, idx = 0;
    double x, y, r0, r1, r2, r3, rm, lrm, a, r, lr;

    r0 = hypot(0 - xc, 0 - yc);
    r1 = hypot(w - 1 - xc, 0 - yc);
    r2 = hypot(0 - xc, h - 1 - yc);
    r3 = hypot(w - 1 - xc, h - 1 - yc);

    rm = MAX(r0, r1);
    rm = MAX(rm, r2);
    rm = MAX(rm, r3);
    lrm = log(rm);

    for (j = 0; j <= AA * (h - 1); ++j) {
        y = j / AA - yc;
        for (i = 0; i <= AA * (w - 1); ++i) {
            x = i / AA - xc;

            a = atan2(y, x) / (2 * M_PI) - .25;
            while (a < 0.)
                ++a;
            ai = (int) (nang * a);

            r = hypot(x, y);
            if (r <= 1)
                lr = 0;
            else
                lr = log(r) / lrm;
            ri = nrad - 1 - (int) ((nrad - 1) * lr);

            k = ri * nang + ai;

            ++num[k];
            lut[idx++] = k;
        }
    }
}

static void polar_intp(swap_bicubic_t * f, const float *in, size_t w, size_t h,
                       guint32 * lut, guint16 * num, float *out)
{
    size_t i, j, k, idx = 0;

    for (j = 0; j <= AA * (h - 1); ++j)
        for (i = 0; i <= AA * (w - 1); ++i) {
            k = lut[idx++];
            out[k] += swap_bicubic(f, in, w, h, i / AA, j / AA) / num[k];
        }
}

#define NANG ((size_t) 1024)
#define NRAD ((size_t) (M_SQRT2 * 1024))
#define XC   511.5
#define YC   511.5

#define FORCE_COMPUTE 0
#define WRITE_FITS    0

#define POLAR_FITS SIDC_INSTALL_LIB "/data/polar.fits"

float *swap_polar(const float *in, size_t w, size_t h, size_t nang, size_t nrad,
                  double xc, double yc)
{
    guint32 *lut;
    guint16 *num;

    size_t lw = (w - 1) * AA + 1, lh = (h - 1) * AA + 1;

    if (FORCE_COMPUTE || AA != 2 || nang != NANG || nrad != NRAD || xc != XC ||
        yc != YC || access(POLAR_FITS, R_OK)) {
        lut = (guint32 *) g_malloc(lw * lh * sizeof *lut);
        num = (guint16 *) g_malloc0(nang * nrad * sizeof *num);

        polar_prep(lut, w, h, num, nang, nrad, xc, yc);

        if (WRITE_FITS) {
            sfts_t *f = sfts_create("polar.fits", NULL);

            sfts_create_image(f, lw, lh, SUINT32);
            sfts_write_image(f, lut, lw, lh, SUINT32);
            sfts_create_image(f, nang, nrad, SUINT16);
            sfts_write_image(f, num, nang, nrad, SUINT16);

            sfts_goto_hdu(f, 1);
            g_free(sfts_free(f));
        }
    } else {
        size_t rw, rh;
        sfts_t *f = sfts_openro(POLAR_FITS);

        lut = (guint32 *) sfts_read_image(f, &rw, &rh, SUINT32);
        if (rw != lw || rh != lh)
            P2SC_Msg(LVL_FATAL_INTERNAL_ERROR,
                     "Size of image read (%zd, %zd) different than expected (%zd, %zd)",
                     rw, rh, lw, lh);
        sfts_goto_hdu(f, 2);
        num = (guint16 *) sfts_read_image(f, &rw, &rh, SUINT16);
        if (rw != nang || rh != nrad)
            P2SC_Msg(LVL_FATAL_INTERNAL_ERROR,
                     "Size of image read (%zd, %zd) different than expected (%zd, %zd)",
                     rw, rh, nang, nrad);

        g_free(sfts_free(f));
    }

    float *out = (float *) g_malloc0(nang * nrad * sizeof *out);
    swap_bicubic_t *f = swap_bicubic_alloc(0, 0.5);

    polar_intp(f, in, w, h, lut, num, out);

    swap_bicubic_free(f);
    g_free(num);
    g_free(lut);

    return out;
}

float *swap_rebin(const float *in, size_t w, size_t h, size_t fx, size_t fy)
{
    size_t ow = w / fx, oh = h / fy, i, j, k, l;

    if (ow != w / (double) fx || oh != h / (double) fy)
        return NULL;

    float f = 1 / (double) (fx * fy);
    float *out = (float *) g_malloc(ow * oh * sizeof *out), *o = out;

    for (j = 0; j < h; j += fy)
        for (i = 0; i < w; i += fx) {
            float s = 0;
            for (l = 0; l < fy; ++l)
                for (k = 0; k < fx; ++k)
                    s += in[(j + l) * w + i + k];
            *o++ = s * f;
        }

    return out;
}

static inline int rnd(double a)
{
    if (a < 0.)
        return a - .5;
    else
        return a + .5;
}

/* check factors for new sizes/maps */
#define AAA 6
#define AAR 4

float *swap_polar2(const float *in, size_t w, size_t h, size_t nang,
                   size_t nrad, double xc, double yc, double R)
{
    size_t i, j, ssa = AAA * nang, ssr = AAR * nrad;
    float *pol = (float *) g_malloc(ssa * ssr * sizeof *pol), *p = pol, *out;
    double *s = (double *) g_malloc(ssa * sizeof *s);
    double *c = (double *) g_malloc(ssa * sizeof *c);

    for (i = 0; i < ssa; ++i) {
        double a = M_PI * (2. / ssa * i + .5);
        c[i] = cos(a);
        s[i] = sin(a);
    }

    R /= M_E - 1;
    for (j = 0; j < ssr; ++j) {
        double r = R * (exp((ssr - 1 - j) / (double) ssr) - 1);

        for (i = 0; i < ssa; ++i) {
            double x = xc + r * c[i], y = yc + r * s[i];
            int ix = rnd(x), iy = rnd(y);

            if (ix < 0 || (size_t) ix >= w || iy < 0 || (size_t) iy >= h)
                *p++ = 0;
            else
                /* nearest neighbour, for small radii use interpolation */
                *p++ = in[iy * w + ix];
        }
    }
    g_free(c);
    g_free(s);

    out = swap_rebin(pol, ssa, ssr, AAA, AAR);
    g_free(pol);

    return out;
}
