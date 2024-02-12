/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

static const char _versionid_[] __attribute__((unused)) = "$Id: jp2img.c 5248 2016-05-18 23:02:15Z bogdan $";

#include <stdio.h>
#include <glib.h>

#include "p2sc_stdlib.h"
#include "swap_color.h"
#include "swap_draw.h"
#include "swap_file.h"
#include "swap_file_j2k.h"
#include "swap_vliet8.h"

#define JPEG_DEFAULT     75
#define PNG_DEF_STRATEGY 3

#define APP_NAME "SWMPG"

static void swap_crispen8(guint8 *, size_t, size_t);

int main(int argc, char **argv) {
    int pgm = 0, png = 0, jpeg = JPEG_DEFAULT;
    char *cm = NULL, *label = NULL;
    GOptionEntry entries[] = {
        { "colormap", 'C', 0, G_OPTION_ARG_STRING, &cm,
         "Use a colormap: hot, jet, aia171", "none" },
        { "label", 'l', 0, G_OPTION_ARG_STRING, &label,
         "Label image with string", "string" },
        { "jpeg", 'j', 0, G_OPTION_ARG_INT, &jpeg,
         "Set JPEG quality level", G_STRINGIFY(JPEG_DEFAULT) },
        { "pgm", 'g', 0, G_OPTION_ARG_NONE, &pgm,
         "Output a PGM file instead of a JPG", NULL },
        { "png", 'n', 0, G_OPTION_ARG_NONE, &png,
         "Output a PNG file instead of a JPG", NULL },
        { NULL, 0, 0, G_OPTION_ARG_NONE, NULL, NULL, NULL }
    };

    p2sc_option(argc, argv, APP_NAME, "FILE - SWAP Media Product Generator",
                "This program generates quicklook images out of Helioviewer JP2's", entries);

    size_t w, h, n;
    guint8 *img = swap_read_j2k(argv[1], &w, &h, &n);
    swap_crispen8(img, w, h);

    if (label != NULL) {
        swap_drawstring(label, img, w, h);
        g_free(label);
    }

    if (img) {
        swap_palette_t *pal = (swap_palette_t *) GINT_TO_POINTER(-1);
        if (n == 1 && !pgm)
            pal = swap_palette_rgb_get(cm);

        if (pgm)
            swap_write_pgm("-", (const guint16 *) img, w, h, 255);
        else if (png)
            swap_write_png("-", img, w, h, pal, NULL, PNG_DEF_STRATEGY);
        else
            swap_write_jpg("-", img, w, h, pal, jpeg, NULL);
        g_free(img);
    }

    return 0;
}

#define SIGMA  1/1.6
#define NARROW 1
#define WIDE   .5
#define BLUR   1

static void swap_crispen8(guint8 *out, size_t w, size_t h) {
    size_t i, l = w * h;
    float *in = (float *) g_malloc(3 * l * sizeof *in);

    swap_gauss8(out, in + 0 * l, w, h, SIGMA);
    swap_gauss8(out, in + 1 * l, w, h, SIGMA * 1.6);
    swap_gauss8(out, in + 2 * l, w, h, SIGMA * 3.2);

    /* DoG narrow & wide */
    for (i = 0; i < l; ++i) {
        float v = out[i] + 0.33 * ((NARROW + WIDE) * in[i + 0 * l] - NARROW * in[i + l] - WIDE * in[i + 2 * l]) + .5;
        out[i] = CLAMP(v, 0, 255);
    }

    g_free(in);
}
