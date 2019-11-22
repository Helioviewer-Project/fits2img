/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

static const char _versionid_[] __attribute__ ((unused)) =
    "$Id: fitsproc.c 4468 2015-04-30 12:51:27Z bogdan $";

#include <string.h>
#include <glib.h>

#include "p2sc_fits.h"
#include "swap_meta.h"

#include "fitsproc.h"

static char *process_header(sfts_t *, const char *);

procfits_t *fitsproc(const char *name, int noverify)
{
    sfts_t *f = sfts_openro(name, noverify ? SFTS_SUM_NOVERIFY : 0);

    sfts_find_hdukey(f, "DATE-OBS");

    procfits_t *p = (procfits_t *) g_malloc0(sizeof *p);
    p->name = g_strdup(name);
    p->dateobs = sfts_read_keystring(f, "DATE-OBS");
    p->telescop = sfts_read_keystring(f, "TELESCOP");
    p->instrume = sfts_read_keystring(f, "INSTRUME");

    p->detector = sfts_read_keystring0(f, "DETECTOR");
    if (!p->detector) {
        g_free(p->detector);
        p->detector = g_strdup(p->instrume);
    }

    p->wavelnth = sfts_read_keystring(f, "WAVELNTH");

    p->xml = process_header(f, "swhv@oma.be");
    p->im = (float *) sfts_read_image(f, &(p->w), &(p->h), SFLOAT);

    g_free(sfts_free(f));

    return p;
}

void procfits_free(procfits_t * p)
{
    if (p) {
        g_free(p->im);
        g_free(p->name);
        g_free(p->dateobs);
        g_free(p->telescop);
        g_free(p->instrume);
        g_free(p->detector);
        g_free(p->wavelnth);
        g_free(p->xml);

        memset(p, 0, sizeof *p);
        g_free(p);
    }
}

static char *process_header(sfts_t *f, const char *contact)
{
    int z1 = -1, z2 = -1;
    int naxis1, naxis2, znaxis1, znaxis2;
    sfkey_t k = {.c = NULL };

    k.k = "ZNAXIS1", k.t = 'I', z1 = sfts_read_keymaybe(f, &k);
    if (z1 == 1)
        znaxis1 = k.v.i;
    k.k = "ZNAXIS2", k.t = 'I', z2 = sfts_read_keymaybe(f, &k);
    if (z2 == 1)
        znaxis2 = k.v.i;

    if (z1 == 1 && z2 == 1) {
        k.k = "NAXIS1", k.t = 'I', sfts_read_key(f, &k);
        naxis1 = k.v.i;
        k.k = "NAXIS2", k.t = 'I', sfts_read_key(f, &k);
        naxis2 = k.v.i;

        if (naxis1 != znaxis1 || naxis2 != znaxis2) {
            sfts_t *f2 = sfts_create(NULL, NULL);

            sfts_copy_header(f, f2);
            k.k = "NAXIS1", k.t = 'I', k.v.i = znaxis1;
            sfts_write_key(f2, &k);
            k.k = "NAXIS2", k.t = 'I', k.v.i = znaxis2;
            sfts_write_key(f2, &k);

            char *ret = swap_fits2hv(f2, contact);
            g_free(sfts_free(f2));

            return ret;
        }
    }

    return swap_fits2hv(f, contact);
}
