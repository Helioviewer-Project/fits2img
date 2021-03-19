/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

static const char _versionid_[] __attribute__((unused)) =
    "$Id: p2sc_time.c 5348 2018-04-25 13:55:17Z bogdan $";

#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <glib.h>

#include "p2sc_math.h"
#include "p2sc_msg.h"
#include "p2sc_time.h"

#define TWENTY_EIGHT (1ULL << 28)

#define REFERENCE_DATE 2018-01-01   // A date before the rollover date
#define REFERENCE_PASS 26206    // A pass number on the day REFERENCE_DATE
#define REFERENCE_OBET 257630618    // The second part of the OBET of REFERENCE_DATE

static void warn_cuc(const guint8 t[SIZEOF_CUC]) {
    static const char hex[] = "0123456789abcdef";
    char obet[2 * SIZEOF_CUC + 1];

    for (size_t i = 0; i < SIZEOF_CUC; ++i) {
        obet[2 * i] = hex[t[i] >> 4];
        obet[2 * i + 1] = hex[t[i] & 0xf];
    }
    obet[2 * SIZEOF_CUC] = 0;

    P2SC_Msg(LVL_WARNING_CORRUPT_INPUT_DATA, "Corrupted OBET: %s", obet);
}

static void warn_cuc_decoded(guint32 s, guint32 f) {
    /* re-encode CUC */
    guint8 b[SIZEOF_CUC];
    guint32 ss = GUINT32_TO_BE(s);

    memcpy(b, &ss, 4);
    b[4] = (f >> 16) & 0xff;
    b[5] = (f >> 8) & 0xff;
    b[6] = f & 0xff;
    warn_cuc(b);
}

guint64 p2sc_cuc2ticks_decoded(int passno, guint32 s, guint32 f) {
    if (f & 0x3ff)
        warn_cuc_decoded(s, f);

    guint64 sec = s;
    if (sec > TWENTY_EIGHT - 1)
        warn_cuc_decoded(s, f);

    if (passno > REFERENCE_PASS && sec < REFERENCE_OBET)
        sec += TWENTY_EIGHT;

    return (sec << 14) + (f >> 10);
}

guint64 p2sc_cuc2ticks(const guint8 t[SIZEOF_CUC], int passno) {
    if (!t)
        return -1;

    guint32 f = (t[4] << 16) + (t[5] << 8) + t[6];

    guint32 s;
    memcpy(&s, t, sizeof s);
    s = GUINT32_FROM_BE(s);

    return p2sc_cuc2ticks_decoded(passno, s, f);
}

guint64 p2sc_gp1obt2ticks(double gp1obt, int passno) {
    if (passno > REFERENCE_PASS && gp1obt < REFERENCE_OBET)
        gp1obt += TWENTY_EIGHT;
    return (guint64) (gp1obt / OBET_RES);
}

int p2sc_ascii2cuc(guint8 t[SIZEOF_CUC], const char *in) {
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

char *p2sc_date2string(const double d[6]) {
    return g_strdup_printf(d[5] < 10 ?
                           "%04d-%02d-%02dT%02d:%02d:0%.3fZ" :
                           "%04d-%02d-%02dT%02d:%02d:%.3fZ",
                           (int) d[0], (int) d[1], (int) d[2], (int) d[3], (int) d[4], d[5]);
}

char *p2sc_timestamp(double t, int prec) {
    if (t < 0) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        t = tv.tv_sec + tv.tv_usec / 1000000.;
        // t = g_get_real_time() / 1000000.; for glib-2.0>=2.28
    }

    double i, f = modf(t, &i);
    time_t s = i;

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

/* Since: 2.56
int p2sc_string2date(const char *iso_date, double d[6])
{
    GDateTime *dt = g_date_time_new_from_iso8601(iso_date, NULL);
    if (!dt)
        return -1;

    d[0] = g_date_time_get_year(dt);
    d[1] = g_date_time_get_month(dt);
    d[2] = g_date_time_get_day_of_month(dt);
    d[3] = g_date_time_get_hour(dt);
    d[4] = g_date_time_get_minute(dt);
    d[5] = g_date_time_get_seconds(dt);

    g_date_time_unref(dt);
    return 0;
}
*/

/* adapted from glib gtimer.c git 2009-06-02 */
int p2sc_string2date(const char *iso_date, double d[6]) {
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
        // YYYY-MM-DD
        year = val;
        date++;
        mon = strtol(date, &date, 10);

        if (*date++ != '-')
            return -1;

        mday = strtol(date, &date, 10);
    } else {
        // YYYYMMDD
        mday = val % 100;
        mon = (val % 10000) / 100;
        year = val / 10000;
    }

    if (*date++ != 'T')
        return -1;

    val = strtol(date, &date, 10);
    if (*date == ':') {
        // hh:mm:ss
        hour = val;
        date++;
        min = strtol(date, &date, 10);

        if (*date++ != ':')
            return -1;

        sec = strtol(date, &date, 10);
    } else {
        // hhmmss
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
