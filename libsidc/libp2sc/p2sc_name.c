/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

static const char _versionid_[] __attribute__((unused)) =
    "$Id: p2sc_name.c 5108 2014-06-19 12:29:23Z bogdan $";

#include <string.h>
#include <glib.h>

#include "p2sc_hash.h"
#include "p2sc_msg.h"
#include "p2sc_stdlib.h"
#include "p2sc_time.h"
#include "p2sc_name.h"

#define DS G_DIR_SEPARATOR_S
#define DN(dir) (dir ? dir : ".")

static char *p2sc_dirtree(const char *dir, const char *dateobs, double d[6]) {
    if (p2sc_string2date(dateobs, d))
        P2SC_Msg(LVL_FATAL, "p2sc_string2time(%s)", dateobs);

    return g_strdup_printf("%s" DS "%04d" DS "%02d" DS "%02d", DN(dir),
                           (int) d[0], (int) d[1], (int) d[2]);
}

char *p2sc_name_dirtree(const char *dir, const char *dateobs) {
    double d[6];
    return p2sc_dirtree(dir, dateobs, d);
}

char *p2sc_name_swap_tmr(const char *dir, const guint8 *data, size_t len, guint64 o) {
    guint32 crc = p2sc_crc32_finalise(p2sc_crc32(0, data, len), len);
    char acrc[2 * sizeof crc + 1];

    sprintf(acrc, "%08x", crc);
    return g_strdup_printf("%s" DS "swap_" P_OBET_F "_%s.fits", DN(dir), o, acrc);
}

char *p2sc_name_swap_lv0(const char *dir, const char *dateobs) {
    double d[6];
    char *tri = p2sc_dirtree(dir, dateobs, d);
    char *ret = g_strdup_printf("%s" DS "swap_lv0_%04d%02d%02d_%02d%02d%02d.fits", tri,
                                (int) d[0], (int) d[1], (int) d[2], (int) d[3],
                                (int) d[4], (int) d[5]);

    g_free(tri);
    return ret;
}

char *p2sc_name_swap_qlk(const char *dir, const char *name, const char *ext) {
    char *ret, *base = g_path_get_basename(name);

    if ((ret = strrchr(base, '.')))
        *ret = 0;

    ret = g_strdup_printf("%s" DS "%s.%s", DN(dir), base, ext);
    g_free(base);

    return ret;
}

static char *p2sc_strdup_replace(const char *str, char oc, char nc) {
    size_t l = strlen(str), i;
    char *ret = (char *) g_malloc(l + 1);

    for (i = 0; i < l; ++i)
        if (str[i] != oc)
            ret[i] = str[i];
        else
            ret[i] = nc;
    ret[l] = 0;

    return ret;
}

char *p2sc_name_swap_jhv(const char *dateobs, const char *telescop,
                         const char *instrume, const char *detector, const char *wavelnth) {
    double d[6];
    if (p2sc_string2date(dateobs, d))
        P2SC_Msg(LVL_FATAL, "p2sc_string2date(%s)", dateobs);

    /* avoid SDO/AIA */
    char *tele = p2sc_strdup_replace(telescop, '/', '-');
    /* avoid SUVI instrument name */
    char *inst = p2sc_strdup_replace(instrume, ' ', '_');
    /* final . to protect fractional wavelengths */
    char *ret = g_strdup_printf("%04d_%02d_%02d__%02d_%02d_%02d__%s_%s_%s_%s.",
                                (int) d[0], (int) d[1], (int) d[2], (int) d[3],
                                (int) d[4], (int) d[5],
                                tele, inst, detector, wavelnth);
    g_free(inst);
    g_free(tele);

    return ret;
}
