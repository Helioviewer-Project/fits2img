/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

static const char _versionid_[] __attribute__((unused)) =
    "$Id: swap_draw.c 5113 2014-06-19 15:07:34Z bogdan $";

#include <string.h>
#include <glib.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include "swap_draw.h"

/* [[1,2,1], [2,4,2], [1,2,1]] */
static void blur(guint8 *buf, int w, int h) {
    int x, y;
    unsigned int osum, nsum;

    for (y = 0; y < h; ++y) {
        osum = 2 * buf[y * w];
        for (x = 0; x < w - 1; ++x) {
            nsum = buf[y * w + x] + buf[y * w + x + 1];
            buf[y * w + x] = (osum + nsum) >> 2;
            osum = nsum;
        }
    }

    for (x = 0; x < w; ++x) {
        osum = 2 * buf[x];
        for (y = 0; y < h - 1; ++y) {
            nsum = buf[y * w + x] + buf[(y + 1) * w + x];
            buf[y * w + x] = (osum + nsum) >> 2;
            osum = nsum;
        }
    }
}

typedef struct {
    guint8 *b;
    int w;
    int h;
} bmap_t;

static bmap_t *new_bmap(int w, int h) {
    bmap_t *b = (bmap_t *) g_malloc(sizeof *b);

    b->b = (guint8 *) g_malloc(w * h);
    b->w = w;
    b->h = h;
    return b;
}

static void del_bmap(bmap_t *b) {
    if (b) {
        g_free(b->b);
        g_free(b);
    }
}

static void ft2bmap(FT_Bitmap *bt, bmap_t *bm, int bord) {
    int i, w = bt->width, h = bt->rows;
    guint8 *src = bt->buffer, *dst = bm->b + bord * bm->w + bord;

    memset(bm->b, 0, bm->w * bm->h);
    for (i = 0; i < h; ++i) {
        memcpy(dst, src, w);
        src += bt->pitch;
        dst += bm->w;
    }
}

#define _CHK(_a, _b) \
    if ((_a) < 0) \
        continue; \
    else if ((_a) > (int) (_b) - 1) \
        break \

static inline void draw_bmap(bmap_t *bm, int x, int y, guint8 *im, size_t w, size_t h) {
    for (int j = 0; j < bm->h; ++j) {
        int yy = y + j;
        _CHK(yy, h);
        for (int i = 0; i < bm->w; ++i) {
            int xx = x + i;
            _CHK(xx, w);

            int p = bm->b[bm->w * j + i];
            p += im[yy * w + xx];
            if (p > 255)
                p = 255;
            im[yy * w + xx] = p;
        }
    }
}

static inline void draw_bmap_inv(bmap_t *bm, int x, int y, guint8 *im, size_t w, size_t h) {
    for (int j = 0; j < bm->h; ++j) {
        int yy = y + j;
        _CHK(yy, h);
        for (int i = 0; i < bm->w; ++i) {
            int xx = x + i;
            _CHK(xx, w);

            int p = bm->b[bm->w * j + i];
            p = (int) im[yy * w + xx] - p;
            if (p < 0)
                p = 0;
            im[yy * w + xx] = p;
        }
    }
}

/* draw blurred to reduce bits allocated to timestamp */
/* draw grey shadow to be visible on light background */
static void draw_glyph(FT_Glyph glyph, int x, int y, guint8 *im, size_t w, size_t h) {
    if (FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_LIGHT, 0, 0))
        return;

    FT_BitmapGlyph bg = (FT_BitmapGlyph) glyph;
    FT_Bitmap *bt = &bg->bitmap;

    if (bt->pixel_mode != FT_PIXEL_MODE_GRAY || bt->num_grays != 256) {
        FT_Done_Glyph(glyph);
        return;
    }

    x += bg->left - 1;          /* tighten up a bit */
    y -= bg->top;

    bmap_t *bm = new_bmap(bt->width + 4, bt->rows + 4);

    ft2bmap(bt, bm, 2);
    blur(bm->b, bm->w, bm->h);
    blur(bm->b, bm->w, bm->h);
    draw_bmap_inv(bm, x + 1, y + 1, im, w, h);

    ft2bmap(bt, bm, 2);
    blur(bm->b, bm->w, bm->h);
    draw_bmap(bm, x, y, im, w, h);

    del_bmap(bm);
    FT_Done_Glyph(glyph);
}

static void draw_string(const char *font, size_t size,
                        const char *text, int x, int y, guint8 *im, size_t w, size_t h) {
    FT_Library lib;
    FT_Face face;

    if (!text || FT_Init_FreeType(&lib))
        return;

    if (FT_New_Face(lib, font, 0, &face) || FT_Set_Pixel_Sizes(face, 0, size))
        goto end;

    FT_Glyph glyph;
    FT_GlyphSlot slot = face->glyph;
    size_t i, len = strlen(text);

    for (i = 0; i < len; ++i) {
        FT_UInt idx = FT_Get_Char_Index(face, text[i]);

        if (FT_Load_Glyph(face, idx, FT_LOAD_DEFAULT) || FT_Get_Glyph(slot, &glyph))
            continue;

        draw_glyph(glyph, x, y, im, w, h);
        x += slot->advance.x >> 6;
        FT_Done_Glyph(glyph);
    }

  end:
    FT_Done_FreeType(lib);
}

#define ROUND_DOWNTO(a, quanta) ((a) & ~((quanta) - 1))
#define ROUND_UPTO(a, quanta)   (((a) + ((quanta) - 1)) & ~((quanta) - 1))

#define P2SC_FONT_REGULAR   SIDC_INSTALL_LIB "/data/LiberationSans-Regular.ttf"
#define P2SC_FONT_DROIDBOLD SIDC_INSTALL_LIB "/data/DroidSans-Bold.ttf"

void swap_drawstring(const char *str, guint8 *im, size_t w, size_t h) {
    size_t fs = ROUND_UPTO(h / 32, 4);
    size_t bd = CLAMP(h / 64, 8, 16);
    draw_string(P2SC_FONT_DROIDBOLD, CLAMP(fs, 20, 32), str, bd, h - 3 - bd, im, w, h);
}
