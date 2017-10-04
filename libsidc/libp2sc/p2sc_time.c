/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

static const char _versionid_[] __attribute__ ((unused)) =
    "$Id: p2sc_time.c 5108 2014-06-19 12:29:23Z bogdan $";

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <glib.h>

#include "p2sc_math.h"
#include "p2sc_msg.h"
#include "p2sc_time.h"

guint64 p2sc_cuc2ticks(const guint8 t[SIZEOF_CUC])
{
    if (!t)
        return -1;

    guint32 s, f = (t[4] << 16) + (t[5] << 8) + t[6];

    if (f & 0x3ff) {
        static const char hex[] = "0123456789abcdef";
        char obet[2 * SIZEOF_CUC + 1];

        for (size_t i = 0; i < SIZEOF_CUC; ++i) {
            obet[2 * i] = hex[t[i] >> 4];
            obet[2 * i + 1] = hex[t[i] & 0xf];
        }
        obet[2 * SIZEOF_CUC] = 0;

        P2SC_Msg(LVL_WARNING_CORRUPT_INPUT_DATA, "Corrupted OBET: %s", obet);
    }

    memcpy(&s, t, sizeof s);

    return (((guint64) GUINT32_FROM_BE(s)) << 14) + (f >> 10);
}

int p2sc_ascii2cuc(guint8 t[SIZEOF_CUC], const char *in)
{
    guint8 c, i = 0;

    if (!t || !in || strlen(in) != 2 * SIZEOF_CUC)
        return -1;

    while ((c = *in++)) {
        if (c >= '0' && c <= '9')
            c -= '0';
        else {
            if (c >= 'A' && c <= 'F')
                c -= 'A' - 10;
            else if (c >= 'a' && c <= 'f')
                c -= 'a' - 10;
            else
                return -1;
        }

        if (i++ & 1)
            *t++ += c;
        else
            *t = c << 4;
    }

    return 0;
}

char *p2sc_date2string(const double d[6])
{
    return g_strdup_printf(d[5] < 10 ?
                           "%04d-%02d-%02dT%02d:%02d:0%.3fZ" :
                           "%04d-%02d-%02dT%02d:%02d:%.3fZ",
                           (int) d[0], (int) d[1], (int) d[2],
                           (int) d[3], (int) d[4], d[5]);
}

char *p2sc_timestamp(double t, int prec)
{
    time_t s;
    double f;

    if (t < 0) {
        GTimeVal tv;

        g_get_current_time(&tv);
        s = tv.tv_sec;
        f = tv.tv_usec / 1000000.;
    } else {
        double i;

        f = modf(t, &i);
        s = i;
    }

    f = p2sc_round(f, prec);
    if (f >= 1) {
        ++s;
        f = 0;
    }

    struct tm *tm = gmtime(&s);
    if (!tm)
        P2SC_Msg(LVL_FATAL, "gmtime(%f)", (double) s);

    double tt[6] = {
        tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
        tm->tm_hour, tm->tm_min, tm->tm_sec + f
    };

    return p2sc_date2string(tt);
}

/* adapted from glib gtimer.c git 2009-06-02 */
int p2sc_string2date(const char *iso_date, double d[6])
{
    char *date = (char *) iso_date;
    long year, mon, mday, hour, min, sec, usec = 0;
    long val;

    while (g_ascii_isspace(*date))
        date++;

    if (*date == '\0')
        return -1;

    if (!g_ascii_isdigit(*date) && *date != '-' && *date != '+')
        return -1;

    val = strtol(date, &date, 10);
    if (*date == '-') {
        /* YYYY-MM-DD */
        year = val;
        date++;
        mon = strtol(date, &date, 10);

        if (*date++ != '-')
            return -1;

        mday = strtol(date, &date, 10);
    } else {
        /* YYYYMMDD */
        mday = val % 100;
        mon = (val % 10000) / 100;
        year = val / 10000;
    }

    if (*date++ != 'T')
        return -1;

    val = strtol(date, &date, 10);
    if (*date == ':') {
        /* hh:mm:ss */
        hour = val;
        date++;
        min = strtol(date, &date, 10);

        if (*date++ != ':')
            return -1;

        sec = strtol(date, &date, 10);
    } else {
        /* hhmmss */
        sec = val % 100;
        min = (val % 10000) / 100;
        hour = val / 10000;
    }

    if (*date == '.') {
        glong mul = 100000;

        while (g_ascii_isdigit(*++date)) {
            usec += (*date - '0') * mul;
            mul /= 10;
        }
        if (sec < 0)
            usec = -usec;
    }

    d[0] = year;
    d[1] = mon;
    d[2] = mday;
    d[3] = hour;
    d[4] = min;
    d[5] = sec + usec / 1000000.;

    return 0;
}
