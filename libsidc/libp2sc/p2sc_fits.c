/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

static const char _versionid_[] __attribute__((unused)) =
    "$Id: p2sc_fits.c 5204 2015-04-30 19:12:42Z bogdan $";

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <fitsio.h>

#include "p2sc_file.h"
#include "p2sc_fits.h"
#include "p2sc_msg.h"
#include "p2sc_stdlib.h"
#include "p2sc_time.h"

#define CHK_FTS(f) \
    do { \
        if (f && f->stat) { \
            char _msg[31], *_base = f->name ? g_path_get_basename(f->name) : g_strdup("memory"); \
    \
            fits_get_errstatus(f->stat, _msg); \
            _msg[30] = 0; \
            P2SC_Msg(LVL_FATAL_FITS, "FITS: %s: %s", _msg, _base); \
            g_free(_base); \
        } \
    } while (0)

struct sfts_t {
    fitsfile *fts;
    void *ptr;
    char *name;
    size_t size;
    int stat;
};

static int compare_fits(sfts_t *f) {
    sfts_t *of = sfts_openro(f->name);
    sfkey_t k = { "DATASUM", 'S',.v.s = NULL, NULL };
    char sum[SKEY_LEN], osum[SKEY_LEN];

    int diff = 1, i, num = sfts_get_nhdus(f);
    if (num != sfts_get_nhdus(of))
        goto end;

    for (i = 1; i <= num; ++i) {
        sum[0] = osum[0] = 0;

        sfts_goto_hdu(f, i);
        sfts_goto_hdu(of, i);

        k.v.s = sum;
        sfts_read_key(f, &k);
        k.v.s = osum;
        sfts_read_key(of, &k);

        if (strcmp(sum, osum))
            goto end;
    }
    diff = 0;

  end:
    g_free(sfts_free(of));
    sfts_goto_hdu(f, 1);

    return diff;
}

static char *new_name(const char *name) {
    int i = 0;
    char *dir, *base, *bases0, *ret;
    char **bases;

    dir = g_path_get_dirname(name);
    base = g_path_get_basename(name);

    bases = g_strsplit(base, ".", 3);
    g_free(base);

    if (g_strv_length(bases) > 1 && sscanf(bases[1], "%03d", &i) == 1) {
        if (i == 999)
            P2SC_Msg(LVL_FATAL_INTERNAL_ERROR, "i == 999");
        g_free(bases[1]);
        bases[1] = g_strdup_printf("%03d", ++i);
    } else {
        bases0 = g_strdup_printf("%s.%03d", bases[0], ++i);
        g_free(bases[0]);
        bases[0] = bases0;
    }

    base = g_strjoinv(".", bases);
    g_strfreev(bases);

    ret = g_build_filename(dir, base, NULL);

    g_free(base);
    g_free(dir);

    return ret;
}

static int commit_fits(sfts_t *f, int nuke) {
    int *s = &f->stat;

    char *str;
    sfkey_t k = { NULL, 'S',.v.s = NULL, NULL };

    /* final name of file */
    str = g_path_get_basename(f->name);
    k.k = "FILENAME", k.v.s = str;
    sfts_write_key(f, &k);
    g_free(str);

    str = p2sc_timestamp(-1, 3);
    k.k = "DATE", k.v.s = sfts_timestamp(str);
    sfts_write_key(f, &k);
    g_free(str);

    fits_update_chksum(f->fts, s);
    fits_flush_buffer(f->fts, 0, s);
    /* ensure we're error free at this point */
    CHK_FTS(f);

    /* finally write file */
    return p2sc_create_file(nuke, f->name, f->ptr, f->size);
}

char *sfts_free(sfts_t *f) {
    char *ret = NULL;

    if (f) {
        int *s = &f->stat;

        /* temporary file in memory */
        if (!f->name) {
            fits_close_file(f->fts, s);
            CHK_FTS(f);
            return NULL;
        }

        /* commit memory file to disk */
        if (f->ptr) {
            /* write history & chksum */
            fits_write_chksum(f->fts, s);
            const char *h = p2sc_get_string("history");
            if (h)
                fits_write_history(f->fts, h, s);

          restart:
            if (commit_fits(f, 0)) {
                /* identical data, will replace */
                if (!compare_fits(f))
                    commit_fits(f, 1);
                else {
                    char *name = new_name(f->name);

                    g_free(f->name);
                    f->name = name;
                    goto restart;
                }
            }
        }

        fits_close_file(f->fts, s);
        CHK_FTS(f);

        ret = g_strdup(f->name);

        g_free(f->ptr);
        g_free(f->name);
        memset(f, 0, sizeof *f);
        g_free(f);
    }

    return ret;
}

sfts_t *sfts_create(const char *name, const char *tmpl) {
    sfts_t *f = (sfts_t *) g_malloc0(sizeof *f);
    int *s = &f->stat;

    if (name)
        f->name = g_strdup(name);
    fits_create_memfile(&f->fts, &f->ptr, &f->size, 0, g_realloc, s);

    if (tmpl) {
        int t, st = 0;
        char *line, msg[31], card[SKEY_LEN];
        p2sc_iofile_t *io = p2sc_open_iofile(tmpl, "r");

        while ((line = p2sc_read_line(io))) {
            memset(card, 0, sizeof card);
            fits_parse_template(line, card, &t, &st);
            if (st) {
                fits_get_errstatus(st, msg);
                msg[30] = 0;
                P2SC_Msg(LVL_FATAL_INTERNAL_ERROR, "%s: error parsing: %s - %s", tmpl, line, msg);
            }
            g_free(line);

            if (card[0]) {
                card[80] = 0;
                fits_write_record(f->fts, card, s);
            }
        }
        p2sc_free_iofile(io);
    }
    CHK_FTS(f);

    return f;
}

sfts_t *sfts_openro(const char *name, ...) {
    sfts_t *f = (sfts_t *) g_malloc0(sizeof *f);
    int *s = &f->stat, no_verify = 0;

    f->name = g_strdup(name);
    fits_open_file(&f->fts, name, READONLY, s);

    va_list args;

    va_start(args, name);
    no_verify = va_arg(args, int);
    va_end(args);

    /* verify checksums */
    if (no_verify != SFTS_SUM_NOVERIFY) {
        int dataok, hduok, i, num = sfts_get_nhdus(f);
        for (i = 1; i <= num; ++i) {
            sfts_goto_hdu(f, i);
            fits_verify_chksum(f->fts, &dataok, &hduok, s);
            CHK_FTS(f);

            if (dataok == -1)
                P2SC_Msg(LVL_FATAL_FITS, "FITS: incorrect DATASUM: %s", name);
            if (hduok == -1)
                P2SC_Msg(LVL_FATAL_FITS, "FITS: incorrect CHECKSUM: %s", name);
        }
        sfts_goto_hdu(f, 1);
    }

    return f;
}

int sfts_get_nhdus(sfts_t *f) {
    int num;

    fits_get_num_hdus(f->fts, &num, &f->stat);
    CHK_FTS(f);
    return num;
}

void sfts_goto_hdu(sfts_t *f, int h) {
    int t;

    fits_movabs_hdu(f->fts, h, &t, &f->stat);
    CHK_FTS(f);
}

void sfts_copy(sfts_t *in, sfts_t *out) {
    fits_copy_file(in->fts, out->fts, 1, 1, 1, &out->stat);
    sfts_goto_hdu(out, 1);
}

void sfts_copy_header(sfts_t *in, sfts_t *out) {
    fits_copy_header(in->fts, out->fts, &out->stat);
    CHK_FTS(out);
}

void *sfts_read_image(sfts_t *f, size_t *ww, size_t *hh, int t) {
    int *s = &f->stat, naxis = 0, ft, ls;
    long axes[] = { 1, 1 };
    long pels[] = { 1, 1 };

    fits_get_img_dim(f->fts, &naxis, s);
    if (naxis != 2)
        P2SC_Msg(LVL_FATAL_FITS, "FITS: only 2D images supported: NAXIS=%d", naxis);
    fits_get_img_size(f->fts, 2, axes, s);

    size_t w = axes[0], h = axes[1];

    switch (t) {
    case SUINT16:
        ft = TUSHORT;
        ls = w * sizeof(guint16);
        break;
    case SUINT32:
        ft = TUINT;
        ls = w * sizeof(guint32);
        break;
    case SINT32:
        ft = TINT;
        ls = w * sizeof(gint32);
        break;
    case SFLOAT:
        ft = TFLOAT;
        ls = w * sizeof(float);
        break;
    case SDOUBLE:
        ft = TDOUBLE;
        ls = w * sizeof(double);
        break;
    default:
        P2SC_Msg(LVL_FATAL_FITS, "FITS: data type not implemented: %d", t);
        return NULL;
    }

    void *pix = g_malloc(h * ls);
    fits_read_pix(f->fts, ft, pels, w * h, NULL, pix, NULL, s);
    CHK_FTS(f);

    void *line = g_malloc(ls);
    for (size_t j = 0; j < h / 2; ++j) {
        void *first = (guint8 *) pix + j * ls;
        void *last = (guint8 *) pix + (h - 1 - j) * ls;

        memcpy(line, last, ls);
        memcpy(last, first, ls);
        memcpy(first, line, ls);
    }
    g_free(line);

    *ww = w;
    *hh = h;
    return pix;
}

void sfts_create_image(sfts_t *f, size_t w, size_t h, int t) {
    long ft, naxes[] = { w, h };

    switch (t) {
    case SUINT16:
        ft = USHORT_IMG;
        break;
    case SUINT32:
        ft = ULONG_IMG;
        break;
    case SINT16:
        ft = SHORT_IMG;
        break;
    case SINT32:
        ft = LONG_IMG;
        break;
    case SFLOAT:
        ft = FLOAT_IMG;
        break;
    case SDOUBLE:
        ft = DOUBLE_IMG;
        break;
    default:
        P2SC_Msg(LVL_FATAL_FITS, "FITS: data type not implemented: %d", t);
        return;
    }

    fits_create_img(f->fts, ft, 2, naxes, &f->stat);
    CHK_FTS(f);
}

void sfts_write_image(sfts_t *f, const void *pix, size_t w, size_t h, int t) {
    int *s = &f->stat, ft, ls;
    long pels[] = { 1, 1 };

    switch (t) {
    case SUINT16:
        ft = TUSHORT;
        ls = w * sizeof(guint16);
        break;
    case SUINT32:
        ft = TUINT;
        ls = w * sizeof(guint32);
        break;
    case SINT16:
        ft = TSHORT;
        ls = w * sizeof(gint16);
        break;
    case SINT32:
        ft = TINT;
        ls = w * sizeof(gint32);
        break;
    case SFLOAT:
        ft = TFLOAT;
        ls = w * sizeof(float);
        break;
    case SDOUBLE:
        ft = TDOUBLE;
        ls = w * sizeof(double);
        break;
    default:
        P2SC_Msg(LVL_FATAL_FITS, "FITS: data type not implemented: %d", t);
        return;
    }

    void *line = g_malloc(ls);
    for (size_t j = 0; j < h; ++j) {
        pels[1] = j + 1;
        memcpy(line, (const guint8 *) pix + (h - 1 - j) * ls, ls);
        fits_write_pix(f->fts, ft, pels, w, line, s);
    }
    g_free(line);

    fits_write_chksum(f->fts, s);
    CHK_FTS(f);
}

void sfts_write_key(sfts_t *f, const sfkey_t *k) {
    int *s = &f->stat;

    if (!k->k) {
        P2SC_Msg(LVL_FATAL_INTERNAL_ERROR, "NULL keyword");
        return;
    }

    switch (k->t) {
    case 'U':
        /* unknown */
        fits_update_key_null(f->fts, k->k, k->c, s);
        break;
    case 'X':
        /* long string */
        if (!k->v.s)
            fits_update_key_null(f->fts, k->k, k->c, s);
        else
            fits_update_key_longstr(f->fts, k->k, k->v.s, k->c, s);
        break;
    case 'S':
        /* string */
        if (!k->v.s)
            fits_update_key_null(f->fts, k->k, k->c, s);
        else
            fits_update_key(f->fts, TSTRING, k->k, k->v.s, k->c, s);
        break;
    case 'I':
        /* long long */
        fits_update_key(f->fts, TLONGLONG, k->k, /* warning cfits */ (void *) &k->v.i, k->c, s);
        break;
    case 'F':
        /* double */
        fits_update_key(f->fts, TDOUBLE, k->k, /* warning cfits */ (void *) &k->v.f, k->c, s);
        break;
    case 'B':
        /* boolean */
        fits_update_key(f->fts, TLOGICAL, k->k, /* warning cfits */ (void *) &k->v.b, k->c, s);
        break;
    default:
        P2SC_Msg(LVL_FATAL_INTERNAL_ERROR, "Keyword %s: unknown key type %d", k->k, k->t);
        return;
    }
    CHK_FTS(f);
}

void sfts_delete_key(sfts_t *f, const char *key) {
    if (!key) {
        P2SC_Msg(LVL_FATAL_INTERNAL_ERROR, "NULL keyword");
        return;
    }
    fits_delete_key(f->fts, key, &f->stat);
    CHK_FTS(f);
}

void sfts_write_comment(sfts_t *f, const char *cmnt) {
    if (!cmnt) {
        P2SC_Msg(LVL_FATAL_INTERNAL_ERROR, "NULL comment");
        return;
    }
    fits_write_comment(f->fts, cmnt, &f->stat);
    CHK_FTS(f);
}

static int sfts_read_key_internal(sfts_t *f, sfkey_t *k) {
    int *s = &f->stat;

    if (!k->k) {
        P2SC_Msg(LVL_FATAL_INTERNAL_ERROR, "NULL keyword");
        return 0;
    }

    switch (k->t) {
    case 'S':
        *k->v.s = 0;
        fits_read_key(f->fts, TSTRING, k->k, k->v.s, NULL, s);
        break;
    case 'I':
        /* long long */
        k->v.i = 0;
        fits_read_key(f->fts, TLONGLONG, k->k, &k->v.i, NULL, s);
        break;
    case 'F':
        /* double */
        k->v.f = 0;
        fits_read_key(f->fts, TDOUBLE, k->k, &k->v.f, NULL, s);
        break;
    default:
        P2SC_Msg(LVL_FATAL_INTERNAL_ERROR, "Keyword %s: unknown key type %d", k->k, k->t);
        return 0;
    }

    if (*s == VALUE_UNDEFINED) {
        *s = 0;
        return 0;
    }
    return 1;
}

int sfts_read_key(sfts_t *f, sfkey_t *k) {
    int r = sfts_read_key_internal(f, k);
    CHK_FTS(f);
    return r;
}

int sfts_read_keymaybe(sfts_t *f, sfkey_t *k) {
    int r = sfts_read_key_internal(f, k);
    if (f->stat == KEY_NO_EXIST) {
        f->stat = 0;
        return -1;
    }

    CHK_FTS(f);
    return r;
}

char *sfts_read_keystring0(sfts_t *f, const char *key) {
    sfkey_t k = {.c = NULL };
    char *ret = (char *) g_malloc0(SKEY_LEN);
    k.k = key, k.t = 'S', k.v.s = ret;
    if (sfts_read_keymaybe(f, &k) != 1) {
        g_free(ret);
        ret = NULL;
    }

    return ret;
}

char *sfts_read_keystring(sfts_t *f, const char *key) {
    char *ret = sfts_read_keystring0(f, key);
    if (!ret) {
        char *_base = f->name ? g_path_get_basename(f->name) : g_strdup("memory");
        P2SC_Msg(LVL_FATAL_FITS, "FITS: %s is NULL: %s", key, _base);
    }
    return ret;
}

void sfts_find_hdukey(sfts_t *f, const char *key) {
    int num = sfts_get_nhdus(f);
    for (int i = 1; i <= num; ++i) {
        sfts_goto_hdu(f, i);
        char *val = sfts_read_keystring0(f, key);
        if (val == NULL)
            continue;
        else {
            g_free(val);
            break;
        }
    }
    CHK_FTS(f);
}

static char *clean_string(char *s) {
    size_t i, l = strlen(s);

    // replace double quotes with simple quotes
    for (i = 0; i < l; ++i)
        if (s[i] == '\"')
            s[i] = '\'';
    // remove start/end quotes
    if (s[0] == '\'')
        s[0] = ' ';
    if (l > 0 && s[l - 1] == '\'')
        s[l - 1] = ' ';

    return g_strescape(g_strstrip(s), NULL);
}

char *sfts_read_history(sfts_t *f) {
    char c[SKEY_LEN], k[SKEY_LEN], *ret;
    GString *str = g_string_sized_new(SKEY_LEN);
    int *s = &f->stat, n, i, l;

    fits_get_hdrpos(f->fts, &n, &i, s);

    g_string_append_c(str, '\"');
    i = 0;
    while (i < n) {
        fits_read_record(f->fts, ++i, c, s);
        fits_get_keyname(c, k, &l, s);
        if (!strcmp(k, "HISTORY")) {
            char *v = clean_string(c + 8);
            g_string_append_printf(str, " %s", v);
            g_free(v);
        }
    }
    CHK_FTS(f);
    g_string_append_c(str, '\"');

    ret = str->str;
    g_string_free(str, FALSE);

    return ret;
}

void sfts_head2str(sfts_t *f, char ***keys, char ***vals, char ***coms) {
    int nkeys, *s = &f->stat;
    fits_get_hdrspace(f->fts, &nkeys, NULL, s);

    GArray *ak = g_array_sized_new(TRUE, FALSE, sizeof **keys, nkeys);
    GArray *av = g_array_sized_new(TRUE, FALSE, sizeof **vals, nkeys);
    GArray *ac = g_array_sized_new(TRUE, FALSE, sizeof **coms, nkeys);

    char *str, *longstr;
    char skey[SKEY_LEN], val[SKEY_LEN], com[SKEY_LEN];
    for (int i = 1; i <= nkeys; ++i) {
        fits_read_keyn(f->fts, i, skey, val, com, s);

        char *key = g_strstrip(skey);
        if (!strcmp("CONTINUE", key))
            continue;
        if (key[0] == 0)
            key = "COMMENT";

        int regular = strcmp("HISTORY", key) && strcmp("COMMENT", key);
        if (regular) {
            fits_read_key_longstr(f->fts, key, &longstr, com, s);
            // long strings may end in &, e.g., EUI FILE_RAW
            int len = strlen(longstr);
            if (len > 68 && longstr[len - 1] == '&')
                longstr[len - 1] = 0;
        }

        str = g_strdup(key);
        g_array_append_val(ak, str);
        str = g_strdup(regular ? longstr : val);
        g_array_append_val(av, str);
        str = g_strdup(com);
        g_array_append_val(ac, str);

        if (regular)
            fits_free_memory(longstr, s);
    }
    CHK_FTS(f);

    *keys = (char **) g_array_free(ak, FALSE);
    *vals = (char **) g_array_free(av, FALSE);
    *coms = (char **) g_array_free(ac, FALSE);
}

char *sfts_timestamp(char *ts) {
    size_t l;

    if (!ts)
        return ts;
    if ((l = strlen(ts)) && ts[l - 1] == 'Z')
        ts[l - 1] = 0;
    return ts;
}
