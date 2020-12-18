/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

static const char _versionid_[] __attribute__((unused)) =
    "$Id: p2sc_math.c 5108 2014-06-19 12:29:23Z bogdan $";

#include <math.h>
#include <string.h>
#include <stdlib.h>

#include <glib.h>

#include "p2sc_math.h"
#include "p2sc_msg.h"

#define UPSIDE_DOWN 180.
#define MOUNT_ROLL   90.        /* some people say 92 */

double p2sc_swaproll(double lar) {
    int ilar = lar * 1000;
    double a = UPSIDE_DOWN + MOUNT_ROLL;

    switch (ilar) {
    case 0:
        a += 0;
        break;
    case -707:
        a += 90;
        break;
    case 1000:
        a += 180;
        break;
    case 707:
        a += 270;
        break;
    default:
        P2SC_Msg(LVL_FATAL_CORRUPT_INPUT_DATA, "Unknown LAR quaternion: %g", lar);
    }

    while (a >= 360)
        a -= 360;

    return a;
}

static inline double ipow10(int power) {
    static const double powers[] = {
        1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8,
        1e9, 1e10, 1e11, 1e12, 1e13, 1e14, 1e15
    };

    if (power < 0 || power > 15)
        return pow(10., power);

    return powers[power];
}

double p2sc_round(double v, int d) {
    int p;
    double i, f = modf(v, &i);

    if (!f)
        return v;

    /* idea from http://wiki.php.net/rfc/rounding */
    p = 14 - floor(log10(v));
    if (d >= p)
        return v;

    f = round(f * ipow10(p));
    f = round(f / ipow10(p - d));

    return i + f / ipow10(d);
}

/* matrix multiplication code lifted from SPICE for convenience */
static void mtxm(void *m1, void *m2, int ncol1, int nr1r2, int ncol2, void *mout);
static void mxm(void *m1, void *m2, int nrow1, int ncol1, int ncol2, void *mout);
static void mxmt(void *m1, void *m2, int nrow1, int nc1c2, int nrow2, void *mout);

static void mtxm(void *m1, void *m2, int ncol1, int nr1r2, int ncol2, void *mout) {
#define INDEX(width, row, col) ((row)*(width) + (col))

    double innerProduct;
    double *tmpmat, *loc_m1, *loc_m2;

    int col, row, i;

    size_t size = ncol1 * ncol2 * sizeof(double);
    tmpmat = (double *) g_malloc(size);

    loc_m1 = (double *) m1;
    loc_m2 = (double *) m2;

    for (row = 0; row < ncol1; row++)
        for (col = 0; col < ncol2; col++) {
            innerProduct = 0;
            for (i = 0; i < nr1r2; i++)
                innerProduct += loc_m1[INDEX(ncol1, i, row)] * loc_m2[INDEX(ncol2, i, col)];

            tmpmat[INDEX(ncol2, row, col)] = innerProduct;
        }

    memcpy(mout, tmpmat, size);
    g_free(tmpmat);

#undef INDEX
}

static void mxm(void *m1, void *m2, int nrow1, int ncol1, int ncol2, void *mout) {
#define INDEX(width, row, col) ((row)*(width) + (col))

    double innerProduct;
    double *tmpmat, *loc_m1, *loc_m2;

    int col, row, i;

    size_t size = nrow1 * ncol2 * sizeof(double);
    tmpmat = (double *) g_malloc(size);

    loc_m1 = (double *) m1;
    loc_m2 = (double *) m2;

    for (row = 0; row < nrow1; row++)
        for (col = 0; col < ncol2; col++) {
            innerProduct = 0;
            for (i = 0; i < ncol1; i++)
                innerProduct += loc_m1[INDEX(ncol1, row, i)] * loc_m2[INDEX(ncol2, i, col)];

            tmpmat[INDEX(ncol2, row, col)] = innerProduct;
        }

    memcpy(mout, tmpmat, size);
    g_free(tmpmat);

#undef INDEX
}

static void mxmt(void *m1, void *m2, int nrow1, int nc1c2, int nrow2, void *mout) {
#define INDEX(width, row, col) ((row)*(width) + (col))

    double innerProduct;
    double *tmpmat, *loc_m1, *loc_m2;

    int col, row, i;

    size_t size = nrow1 * nrow2 * sizeof(double);
    tmpmat = (double *) g_malloc(size);

    loc_m1 = (double *) m1;
    loc_m2 = (double *) m2;

    for (row = 0; row < nrow1; row++)
        for (col = 0; col < nrow2; col++) {
            innerProduct = 0;
            for (i = 0; i < nc1c2; i++)
                innerProduct += loc_m1[INDEX(nc1c2, row, i)] * loc_m2[INDEX(nc1c2, col, i)];

            tmpmat[INDEX(nrow2, row, col)] = innerProduct;
        }

    memcpy(mout, tmpmat, size);
    g_free(tmpmat);

#undef INDEX
}

void p2sc_dctm(double *m, int N) {
    int i, j;
    double s1 = 1 / sqrt(N), s2 = M_SQRT2 / sqrt(N);

    for (i = 0; i < N; ++i)
        m[i] = s1;
    for (j = 1; j < N; ++j)
        for (i = 0; i < N; ++i)
            m[j * N + i] = s2 * cos(M_PI * (2 * i + 1) * j / (2 * N));
}

void p2sc_fdct(double *n, double *m, int N) {
    mxmt(n, m, N, N, N, n);
    mxm(m, n, N, N, N, n);
}

void p2sc_idct(double *n, double *m, int N) {
    mxm(n, m, N, N, N, n);
    mtxm(m, n, N, N, N, n);
}

/* http://mathworld.wolfram.com/Circle-CircleIntersection.html */
double p2sc_circleoverlap(double R, double r, double d) {
    double f, R2 = R * R, r2 = r * r, d2 = d * d;

    if (d >= R + r)
        return 0;

    f = r2 / R2;

    if (d) {
        double ia;
        double a1 = (d2 + r2 - R2) / (2 * d * r);
        double a2 = (d2 + R2 - r2) / (2 * d * R);

        if (fabs(a1) <= 1 && fabs(a2) <= 1) {
            ia = r2 * acos(a1) + R2 * acos(a2) -
                0.5 * sqrt((-d + r + R) * (d + r - R) * (d - r + R) * (d + r + R));

            f = ia / (M_PI * R2);
        }
    }

    if (f > 1)
        f = 1;

    return f;
}

/* following code is Copyright (C) 1995-2009, Mark Calabretta */
/* included here for convenience */

#define D2R (M_PI / 180)
#define R2D (180 / M_PI)
#define TRIG_TOL 1e-10

double cosd(double angle) {
    int i;

    if (fmod(angle, 90.0) == 0.0) {
        i = abs((int) floor(angle / 90.0 + 0.5)) % 4;
        switch (i) {
        case 0:
            return 1.0;
        case 1:
            return 0.0;
        case 2:
            return -1.0;
        case 3:
            return 0.0;
        }
    }

    return cos(angle * D2R);
}

double sind(double angle) {
    int i;

    if (fmod(angle, 90.0) == 0.0) {
        i = abs((int) floor(angle / 90.0 - 0.5)) % 4;
        switch (i) {
        case 0:
            return 1.0;
        case 1:
            return 0.0;
        case 2:
            return -1.0;
        case 3:
            return 0.0;
        }
    }

    return sin(angle * D2R);
}

void sincosd(double angle, double *s, double *c) {
    int i;

    if (fmod(angle, 90.0) == 0.0) {
        i = abs((int) floor(angle / 90.0 + 0.5)) % 4;
        switch (i) {
        case 0:
            *s = 0.0;
            *c = 1.0;
            return;
        case 1:
            *s = (angle > 0.0) ? 1.0 : -1.0;
            *c = 0.0;
            return;
        case 2:
            *s = 0.0;
            *c = -1.0;
            return;
        case 3:
            *s = (angle > 0.0) ? -1.0 : 1.0;
            *c = 0.0;
            return;
        }
    }

    angle *= D2R;
    *s = sin(angle);
    *c = cos(angle);
}

double tand(double angle) {
    double resid;

    resid = fmod(angle, 360.0);
    if (resid == 0.0 || fabs(resid) == 180.0) {
        return 0.0;
    } else if (resid == 45.0 || resid == 225.0) {
        return 1.0;
    } else if (resid == -135.0 || resid == -315.0) {
        return -1.0;
    }

    return tan(angle * D2R);
}

double acosd(double v) {
    if (v >= 1.0) {
        if (v - 1.0 < TRIG_TOL)
            return 0.0;
    } else if (v == 0.0) {
        return 90.0;
    } else if (v <= -1.0) {
        if (v + 1.0 > -TRIG_TOL)
            return 180.0;
    }

    return acos(v) * R2D;
}

double asind(double v) {
    if (v <= -1.0) {
        if (v + 1.0 > -TRIG_TOL)
            return -90.0;
    } else if (v == 0.0) {
        return 0.0;
    } else if (v >= 1.0) {
        if (v - 1.0 < TRIG_TOL)
            return 90.0;
    }

    return asin(v) * R2D;
}

double atand(double v) {
    if (v == -1.0) {
        return -45.0;
    } else if (v == 0.0) {
        return 0.0;
    } else if (v == 1.0) {
        return 45.0;
    }

    return atan(v) * R2D;
}

double atan2d(double y, double x) {
    if (y == 0.0) {
        if (x >= 0.0) {
            return 0.0;
        } else if (x < 0.0) {
            return 180.0;
        }
    } else if (x == 0.0) {
        if (y > 0.0) {
            return 90.0;
        } else if (y < 0.0) {
            return -90.0;
        }
    }

    return atan2(y, x) * R2D;
}
