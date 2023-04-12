
static const char _versionid_[] __attribute__((unused)) =
    "$Id: swap_vliet8.c 5248 2016-05-18 23:02:15Z bogdan $";

#include <glib.h>

#include "swap_vliet8.h"

#define SRCTYPE const guint8
#define DSTTYPE float

static void YvVfilterCoef(double sigma, double *filter);
static void TriggsM(double *filter, double *M);
static void xline(SRCTYPE * src, DSTTYPE * dest, int sx, int sy, double *filter);
static void yline(DSTTYPE * src, DSTTYPE * dest, int sx, int sy, double *filter);

void swap_gauss8(SRCTYPE *c, DSTTYPE *b, int w, int h, double s) {
    double filter[7];

    /* calculate filter coefficients of x-direction */
    YvVfilterCoef(s, filter);
    /* filter in the x-direction */
    xline(c, b, w, h, filter);
    /* calculate filter coefficients in tanp-direction */
    /* YvVfilterCoef(s, filter); */
    yline(b, b, w, h, filter);
}

static void YvVfilterCoef(double sigma, double *filter) {
    /* the recipe in the Young-van Vliet paper:
     * I.T. Young, L.J. van Vliet, M. van Ginkel, Recursive Gabor filtering.
     * IEEE Trans. Sig. Proc., vol. 50, pp. 2799-2805, 2002.
     *
     * (this is an improvement over Young-Van Vliet, Sig. Proc. 44, 1995)
     */

    double q, qsq;
    double scale;
    double B, b1, b2, b3;

    /* initial values */
    double m0 = 1.16680, m1 = 1.10783, m2 = 1.40586;
    double m1sq = m1 * m1, m2sq = m2 * m2;

    /* calculate q */
    if (sigma < 3.556)
        q = -0.2568 + 0.5784 * sigma + 0.0561 * sigma * sigma;
    else
        q = 2.5091 + 0.9804 * (sigma - 3.556);

    qsq = q * q;

    /* calculate scale, and b[0,1,2,3] */
    scale = (m0 + q) * (m1sq + m2sq + 2 * m1 * q + qsq);
    b1 = -q * (2 * m0 * m1 + m1sq + m2sq + (2 * m0 + 4 * m1) * q + 3 * qsq) / scale;
    b2 = qsq * (m0 + 2 * m1 + 3 * q) / scale;
    b3 = -qsq * q / scale;

    /* calculate B */
    B = (m0 * (m1sq + m2sq)) / scale;

    /* fill in filter */
    filter[0] = -b3;
    filter[1] = -b2;
    filter[2] = -b1;
    filter[3] = B;
    filter[4] = -b1;
    filter[5] = -b2;
    filter[6] = -b3;
}

static void TriggsM(double *filter, double *M) {
    double scale;
    double a1, a2, a3;

    a3 = filter[0];
    a2 = filter[1];
    a1 = filter[2];

    scale = 1.0 / ((1.0 + a1 - a2 + a3) * (1.0 - a1 - a2 - a3) * (1.0 + a2 + (a1 - a3) * a3));
    M[0] = scale * (-a3 * a1 + 1.0 - a3 * a3 - a2);
    M[1] = scale * (a3 + a1) * (a2 + a3 * a1);
    M[2] = scale * a3 * (a1 + a3 * a2);
    M[3] = scale * (a1 + a3 * a2);
    M[4] = -scale * (a2 - 1.0) * (a2 + a3 * a1);
    M[5] = -scale * a3 * (a3 * a1 + a3 * a3 + a2 - 1.0);
    M[6] = scale * (a3 * a1 + a2 + a1 * a1 - a2 * a2);
    M[7] = scale * (a1 * a2 + a3 * a2 * a2 - a1 * a3 * a3 - a3 * a3 * a3 - a3 * a2 + a3);
    M[8] = scale * a3 * (a1 + a3 * a2);
}

/**************************************
 * the low level filtering operations *
 **************************************/

/*
   all filters work in-place on the output buffer (DSTTYPE), except for the
   xline filter, which runs over the input (SRCTYPE)
   Note that the xline filter can also work in-place, in which case src=dst
   and SRCTYPE should be DSTTYPE
*/

static void xline(SRCTYPE *src, DSTTYPE *dest, int sx, int sy, double *filter) {
    int i, j;
    double b1, b2, b3;
    double pix, p1, p2, p3;
    double sum, sumsq;
    double iplus, uplus, vplus;
    double unp, unp1, unp2;
    double M[9];

    sumsq = filter[3];
    sum = sumsq * sumsq;

    for (i = 0; i < sy; i++) {
        /* causal filter */
        b1 = filter[2];
        b2 = filter[1];
        b3 = filter[0];
        p1 = *src / sumsq;
        p2 = p1;
        p3 = p1;

        iplus = src[sx - 1];
        for (j = 0; j < sx; j++) {
            pix = *src++ + b1 * p1 + b2 * p2 + b3 * p3;
            *dest++ = pix;
            p3 = p2;
            p2 = p1;
            p1 = pix;           /* update history */
        }

        /* anti-causal filter */

        /* apply Triggs border condition */
        uplus = iplus / (1.0 - b1 - b2 - b3);
        b1 = filter[4];
        b2 = filter[5];
        b3 = filter[6];
        vplus = uplus / (1.0 - b1 - b2 - b3);

        unp = p1 - uplus;
        unp1 = p2 - uplus;
        unp2 = p3 - uplus;

        TriggsM(filter, M);

        pix = M[0] * unp + M[1] * unp1 + M[2] * unp2 + vplus;
        p1 = M[3] * unp + M[4] * unp1 + M[5] * unp2 + vplus;
        p2 = M[6] * unp + M[7] * unp1 + M[8] * unp2 + vplus;
        pix *= sum;
        p1 *= sum;
        p2 *= sum;

        *(--dest) = pix;
        p3 = p2;
        p2 = p1;
        p1 = pix;

        for (j = sx - 2; j >= 0; j--) {
            pix = sum * *(--dest) + b1 * p1 + b2 * p2 + b3 * p3;
            *dest = pix;
            p3 = p2;
            p2 = p1;
            p1 = pix;
        }
        dest += sx;
    }
}

static void yline(DSTTYPE *src, DSTTYPE *dest, int sx, int sy, double *filter) {
    double *p0, *p1, *p2, *p3, *pswap;
    double *buf0, *buf1, *buf2, *buf3;
    double *uplusbuf;
    int i, j;
    double b1, b2, b3;
    double pix;
    double sum, sumsq;
    double uplus, vplus;
    double unp, unp1, unp2;
    double M[9];

    sumsq = filter[3];
    sum = sumsq * sumsq;

    uplusbuf = (double *) g_malloc(sx * sizeof *uplusbuf);

    buf0 = (double *) g_malloc(sx * sizeof *buf0);
    buf1 = (double *) g_malloc(sx * sizeof *buf1);
    buf2 = (double *) g_malloc(sx * sizeof *buf2);
    buf3 = (double *) g_malloc(sx * sizeof *buf3);

    p0 = buf0;
    p1 = buf1;
    p2 = buf2;
    p3 = buf3;

    /* causal filter */
    b1 = filter[2];
    b2 = filter[1];
    b3 = filter[0];

    /* border first line */
    for (j = 0; j < sx; j++) {
        pix = *src++ / sumsq;
        p1[j] = pix;
        p2[j] = pix;
        p3[j] = pix;
    }
    /* calc last line for Triggs boundary condition */
    src += (sy - 2) * sx;
    for (j = 0; j < sx; j++)
        uplusbuf[j] = *src++ / (1.0 - b1 - b2 - b3);
    src -= sy * sx;

    for (i = 0; i < sy; i++) {
        for (j = 0; j < sx; j++) {
            pix = *src++ + b1 * p1[j] + b2 * p2[j] + b3 * p3[j];
            *dest++ = pix;
            p0[j] = pix;
        }

        /* shift history */
        pswap = p3;
        p3 = p2;
        p2 = p1;
        p1 = p0;
        p0 = pswap;
    }

    /* anti-causal filter */

    /* apply Triggs border condition */
    b1 = filter[4];
    b2 = filter[5];
    b3 = filter[6];
    TriggsM(filter, M);

    /* first line */
    p0 = uplusbuf;
    for (j = sx - 1; j >= 0; j--) {
        uplus = p0[j];
        vplus = uplus / (1.0 - b1 - b2 - b3);

        unp = p1[j] - uplus;
        unp1 = p2[j] - uplus;
        unp2 = p3[j] - uplus;
        pix = M[0] * unp + M[1] * unp1 + M[2] * unp2 + vplus;
        pix *= sum;
        *(--dest) = pix;
        p1[j] = pix;
        pix = M[3] * unp + M[4] * unp1 + M[5] * unp2 + vplus;
        p2[j] = pix * sum;
        pix = M[6] * unp + M[7] * unp1 + M[8] * unp2 + vplus;
        p3[j] = pix * sum;
    }

    for (i = sy - 2; i >= 0; i--) {
        for (j = sx - 1; j >= 0; j--) {
            pix = sum * *(--dest) + b1 * p1[j] + b2 * p2[j] + b3 * p3[j];
            *dest = pix;
            p0[j] = pix;
        }

        /* shift history */
        pswap = p3;
        p3 = p2;
        p2 = p1;
        p1 = p0;
        p0 = pswap;
    }

    g_free(buf0);
    g_free(buf1);
    g_free(buf2);
    g_free(buf3);
    g_free(uplusbuf);
}
