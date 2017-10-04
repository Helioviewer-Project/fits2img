/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

#ifndef __P2SC_MATH_H__
#define __P2SC_MATH_H__

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------- */

    double p2sc_swaproll(double);

    double p2sc_round(double, int);

    static inline void p2sc_mxv(double _m[3][3], double _vi[3], double _vo[3]) {
        double _v[3] = { _vi[0], _vi[1], _vi[2] };

        _vo[0] = _m[0][0] * _v[0] + _m[0][1] * _v[1] + _m[0][2] * _v[2];
        _vo[1] = _m[1][0] * _v[0] + _m[1][1] * _v[1] + _m[1][2] * _v[2];
        _vo[2] = _m[2][0] * _v[0] + _m[2][1] * _v[1] + _m[2][2] * _v[2];
    }

    static inline void p2sc_mtxv(double _m[3][3], double _vi[3], double _vo[3]) {
        double _v[3] = { _vi[0], _vi[1], _vi[2] };

        _vo[0] = _m[0][0] * _v[0] + _m[1][0] * _v[1] + _m[2][0] * _v[2];
        _vo[1] = _m[0][1] * _v[0] + _m[1][1] * _v[1] + _m[2][1] * _v[2];
        _vo[2] = _m[0][2] * _v[0] + _m[1][2] * _v[1] + _m[2][2] * _v[2];
    }

    void p2sc_dctm(double *, int);
    void p2sc_fdct(double *, double *, int);
    void p2sc_idct(double *, double *, int);

    double p2sc_circleoverlap(double, double, double);

    double cosd(double);
    double sind(double);
    void sincosd(double, double *, double *);
    double tand(double);
    double acosd(double);
    double asind(double);
    double atand(double);
    double atan2d(double, double);

/* ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif
#endif
