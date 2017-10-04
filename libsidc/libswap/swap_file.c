/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

static const char _versionid_[] __attribute__ ((unused)) =
    "$Id: swap_file.c 5113 2014-06-19 15:07:34Z bogdan $";

#include <errno.h>
#include <string.h>
#include <png.h>
#include <setjmp.h>
#include <jpeglib.h>
#include <glib.h>

#include "p2sc_file.h"
#include "p2sc_msg.h"
#include "p2sc_stdlib.h"
#include "p2sc_time.h"

#include "swap_color.h"
#include "swap_file.h"

static void png_warning_fn(png_structp png_ptr G_GNUC_UNUSED,
                           png_const_charp msg)
{
    P2SC_Msg(LVL_WARNING_CORRUPT_INPUT_DATA, "libpng: %s", msg);
}

static void png_error_fn(png_structp png_ptr, png_const_charp msg)
{
    /* fatal, subsequent code will not be executed */
    P2SC_Msg(LVL_FATAL_INTERNAL_ERROR, "libpng: %s", msg);
    longjmp(png_jmpbuf(png_ptr), 1);
}

static void png_write_fn(png_structp png_ptr, png_bytep data, png_size_t length)
{
    p2sc_write((p2sc_iofile_t *) png_get_io_ptr(png_ptr), (const char *) data,
               length);
}

static void png_flush_fn(png_structp png_ptr)
{
    p2sc_flush((p2sc_iofile_t *) png_get_io_ptr(png_ptr));
}

static void set_text(png_text * txt, const char *key, const char *value)
{
    memset(txt, 0, sizeof *txt);

    txt->compression = PNG_TEXT_COMPRESSION_NONE;
    txt->key = (png_charp) key;
    txt->text = (png_charp) value;
}

static void set_meta(png_structp png_ptr, png_infop info_ptr)
{
    char *s_time = p2sc_timestamp(-1, 3);
    char *s_soft = g_strdup_printf("%s - %s (%s)", p2sc_get_string("appname"),
                                   p2sc_get_string("prgname"), _versionid_);
    png_text txt[9];

    set_text(txt + 0, "Title", p2sc_get_string("filename"));
    set_text(txt + 1, "Author", "ROB");
    set_text(txt + 2, "Contact", "swap_lyra@oma.be");
    set_text(txt + 3, "Description", "SWAP EUV 17.4nm Sun Image");
    set_text(txt + 4, "Copyright", "Public Domain");
    set_text(txt + 5, "Creation Time", s_time);
    set_text(txt + 6, "Software", s_soft);
    set_text(txt + 7, "Source", g_get_host_name());
    set_text(txt + 8, "Origin", "PROBA2/SWAP");

    png_set_text(png_ptr, info_ptr, txt, G_N_ELEMENTS(txt));

    g_free(s_soft);
    g_free(s_time);
}

#define GRAY_ICC SIDC_INSTALL_LIB "/data/sidc_gray.icc"
#define SRGB_ICC SIDC_INSTALL_LIB "/data/sRGB_IEC61966-2-1_black_scaled.icc"

void swap_write_png(const char *name, const guint8 * in, size_t w, size_t h,
                    swap_palette_t * pal)
{
    const guint8 **rows = NULL;
    p2sc_iofile_t *io = p2sc_open_iofile(name, "w");

    png_structp png_ptr =
        png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, png_error_fn,
                                png_warning_fn);
    if (!png_ptr)
        P2SC_Msg(LVL_FATAL_INTERNAL_ERROR, "PNG initialization error");
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, NULL);
        P2SC_Msg(LVL_FATAL_INTERNAL_ERROR, "PNG initialization error");
    }

    if (setjmp(png_jmpbuf(png_ptr)))
        goto end;

    int color_type = PNG_COLOR_TYPE_GRAY, stride = w;
    if (pal) {
        if (GPOINTER_TO_INT(pal) == -1) {
            color_type = PNG_COLOR_TYPE_RGB;
            stride = 3 * w;
        } else {
            color_type = PNG_COLOR_TYPE_PALETTE;
            png_set_PLTE(png_ptr, info_ptr, (png_color *) pal, 256);
        }
        png_set_sRGB_gAMA_and_cHRM(png_ptr, info_ptr,
                                   PNG_sRGB_INTENT_PERCEPTUAL);
    } else {
        GMappedFile *map = p2sc_map_file(GRAY_ICC);
        png_set_iCCP(png_ptr, info_ptr, "sidc_gray", PNG_COMPRESSION_TYPE_BASE,
                     (gpointer) g_mapped_file_get_contents(map),
                     g_mapped_file_get_length(map));
        g_mapped_file_unref(map);
    }

    png_set_write_fn(png_ptr, io, png_write_fn, png_flush_fn);
    png_set_IHDR(png_ptr, info_ptr, w, h, 8, color_type,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);

    set_meta(png_ptr, info_ptr);

    png_set_compression_mem_level(png_ptr, 9);
    png_set_compression_level(png_ptr, 1);
    png_set_compression_strategy(png_ptr, 3);

    rows = (const guint8 **) g_malloc(h * sizeof *rows);
    for (size_t i = 0; i < h; ++i)
        rows[i] = in + i * stride;
    png_set_rows(png_ptr, info_ptr, (png_bytepp) rows);

    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

  end:
    png_destroy_write_struct(&png_ptr, &info_ptr);
    g_free(rows);
    p2sc_free_iofile(io);
}

struct my_err_mgr {
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

static void jpeg_error_exit(j_common_ptr cinfo)
{
    char msg[JMSG_LENGTH_MAX];

    /* fatal, subsequent code will not be executed */
    cinfo->err->format_message(cinfo, msg);
    P2SC_Msg(LVL_FATAL_INTERNAL_ERROR, "libjpeg: %s", msg);
/*
    struct my_err_mgr *myerr = (struct my_err_mgr *) cinfo->err;
    longjmp(myerr->setjmp_buffer, 1);
*/
}

static void jpeg_output_msg(j_common_ptr cinfo)
{
    char msg[JMSG_LENGTH_MAX];

    cinfo->err->format_message(cinfo, msg);
    P2SC_Msg(LVL_WARNING, "libjpeg: %s", msg);
}

/* This code was originally copied from the jpegicc tool as found in
 * the lcms source code. This code comes with the following copyright
 * notice:
 *
 * Little cms
 * Copyright (C) 1998-2004 Marti Maria
 *
 *  Permission is hereby granted, free of charge, to any person
 *  obtaining a copy of this software and associated documentation
 *  files (the "Software"), to deal in the Software without
 *  restriction, including without limitation the rights to use, copy,
 *  modify, merge, publish, distribute, sublicense, and/or sell copies
 *  of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 *  included in all copies or substantial portions of the Software.
 */

/*
 * Since an ICC profile can be larger than the maximum size of a JPEG
 * marker (64K), we need provisions to split it into multiple markers.
 * The format defined by the ICC specifies one or more APP2 markers
 * containing the following data:
 *
 *    Identifying string    ASCII "ICC_PROFILE\0"  (12 bytes)
 *    Marker sequence number    1 for first APP2, 2 for next, etc (1 byte)
 *    Number of markers    Total number of APP2's used (1 byte)
 *      Profile data        (remainder of APP2 data)

 * Decoders should use the marker sequence numbers to reassemble the
 * profile, rather than assuming that the APP2 markers appear in the
 * correct sequence.
 */

#define ICC_MARKER  (JPEG_APP0 + 2) /* JPEG marker code for ICC */
#define ICC_OVERHEAD_LEN  14    /* size of non-profile data in APP2 */
#define MAX_BYTES_IN_MARKER  65533  /* maximum data len of a JPEG marker */
#define MAX_DATA_BYTES_IN_MARKER  (MAX_BYTES_IN_MARKER - ICC_OVERHEAD_LEN)

/*
 * This routine writes the given ICC profile data into a JPEG file.
 * It *must* be called AFTER calling jpeg_start_compress() and BEFORE
 * the first call to jpeg_write_scanlines().
 * (This ordering ensures that the APP2 marker(s) will appear after the
 * SOI and JFIF or Adobe markers, but before all else.)
 */

static void
jpeg_icc_write_profile(j_compress_ptr cinfo,
                       const unsigned char *icc_data_ptr,
                       unsigned int icc_data_len)
{
    unsigned int num_markers;   /* total number of markers we'll write */
    int cur_marker = 1;         /* per spec, counting starts at 1 */
    unsigned int length;        /* number of bytes to write in this marker */

    /* Calculate the number of markers we'll need, rounding up of course */
    num_markers = icc_data_len / MAX_DATA_BYTES_IN_MARKER;
    if (num_markers * MAX_DATA_BYTES_IN_MARKER != icc_data_len)
        num_markers++;

    int icc_data_rem = icc_data_len;
    while (icc_data_rem > 0) {
        /* length of profile to put in this marker */
        length = icc_data_rem;
        if (length > MAX_DATA_BYTES_IN_MARKER)
            length = MAX_DATA_BYTES_IN_MARKER;
        icc_data_rem -= length;

        /* Write the JPEG marker header (APP2 code and marker length) */
        jpeg_write_m_header(cinfo, ICC_MARKER,
                            (unsigned int) (length + ICC_OVERHEAD_LEN));

        /* Write the marker identifying string "ICC_PROFILE" (null-terminated).
         * We code it in this less-than-transparent way so that the code works
         * even if the local character set is not ASCII.
         */
        jpeg_write_m_byte(cinfo, 0x49);
        jpeg_write_m_byte(cinfo, 0x43);
        jpeg_write_m_byte(cinfo, 0x43);
        jpeg_write_m_byte(cinfo, 0x5F);
        jpeg_write_m_byte(cinfo, 0x50);
        jpeg_write_m_byte(cinfo, 0x52);
        jpeg_write_m_byte(cinfo, 0x4F);
        jpeg_write_m_byte(cinfo, 0x46);
        jpeg_write_m_byte(cinfo, 0x49);
        jpeg_write_m_byte(cinfo, 0x4C);
        jpeg_write_m_byte(cinfo, 0x45);
        jpeg_write_m_byte(cinfo, 0x0);

        /* Add the sequencing info */
        jpeg_write_m_byte(cinfo, cur_marker);
        jpeg_write_m_byte(cinfo, (int) num_markers);

        /* Add the profile data */
        while (length--) {
            jpeg_write_m_byte(cinfo, *icc_data_ptr);
            icc_data_ptr++;
        }
        cur_marker++;
    }
}

void swap_write_jpg(const char *name, const guint8 * in, size_t w, size_t h,
                    swap_palette_t * pal, int scale)
{
    struct my_err_mgr jerr;
    struct jpeg_compress_struct cinfo;
    unsigned char *obuff = NULL, *line = NULL;
    unsigned long osize;

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = jpeg_error_exit;
    jerr.pub.output_message = jpeg_output_msg;

    if (setjmp(jerr.setjmp_buffer))
        goto end;

    const char *icc = GRAY_ICC;
    int ncomps = 1, cspace = JCS_GRAYSCALE, stride = w;
    if (pal) {
        icc = SRGB_ICC;
        ncomps = 3;
        cspace = JCS_RGB;

        if (GPOINTER_TO_INT(pal) == -1)
            stride = 3 * w;
    }

    jpeg_create_compress(&cinfo);
    jpeg_mem_dest(&cinfo, &obuff, &osize);

    cinfo.input_components = ncomps;
    cinfo.in_color_space = cspace;
    jpeg_set_defaults(&cinfo);

    cinfo.dct_method = JDCT_FLOAT;
    cinfo.image_width = w;
    cinfo.image_height = h;
    cinfo.optimize_coding = TRUE;
    /* jpeg_simple_progression(&cinfo); */

    jpeg_set_quality(&cinfo, scale, FALSE);

    jpeg_start_compress(&cinfo, TRUE);

    GMappedFile *map = p2sc_map_file(icc);
    jpeg_icc_write_profile(&cinfo,
                           (const guchar *) g_mapped_file_get_contents(map),
                           g_mapped_file_get_length(map));
    g_mapped_file_unref(map);

    line = (unsigned char *) g_malloc(pal ? 3 * w : w);
    while (h--) {
        if (pal && GPOINTER_TO_INT(pal) != -1) {
            for (size_t i = 0; i < w; ++i) {
                const unsigned char *p = (*pal)[in[i]];
                line[3 * i + 0] = p[0];
                line[3 * i + 1] = p[1];
                line[3 * i + 2] = p[2];
            }
        } else
            memcpy(line, in, stride);

        jpeg_write_scanlines(&cinfo, (JSAMPROW *) & line, 1);
        in += stride;
    }
    jpeg_finish_compress(&cinfo);

    p2sc_iofile_t *io = p2sc_open_iofile(name, "w");
    p2sc_write(io, (const char *) obuff, osize);
    p2sc_free_iofile(io);

  end:
    jpeg_destroy_compress(&cinfo);
    g_free(line);
    g_free(obuff);
}

void swap_write_pgm(const char *name, const guint16 * ptr, size_t w, size_t h,
                    guint16 max)
{
    size_t i, len, over = 0;
    guint16 v;
    void *buf;

    if (max > 255) {
        len = 2 * w * h;
        buf = g_malloc(len);

        for (i = 0; i < w * h; ++i) {
            v = ptr[i];
            if (v > max) {
                ((guint16 *) buf)[i] = GUINT16_TO_BE(max);
                ++over;
            } else
                ((guint16 *) buf)[i] = GUINT16_TO_BE(v);
        }
    } else {
        len = w * h;
        buf = g_malloc(len);

        for (i = 0; i < len; ++i) {
            v = ((const guint8 *) ptr)[i];  /* 1 byte data */
            if (v > max) {
                ((guint8 *) buf)[i] = max;
                ++over;
            } else
                ((guint8 *) buf)[i] = v;
        }
    }

    if (over)
        P2SC_Msg(LVL_WARNING_CORRUPT_INPUT_DATA,
                 "Corrupted image: %zd pixels > %hu", over, max);

    p2sc_iofile_t *io = p2sc_open_iofile(name, "w");
    char *head = g_strdup_printf("P5\n%zd %zd\n%hu\n", w, h, max);
    p2sc_write(io, head, strlen(head));
    p2sc_write(io, (const char *) buf, len);
    p2sc_free_iofile(io);

    g_free(head);
    g_free(buf);
}

static int blank(FILE * f, const char *name)
{
    int c;

    do {
        c = fgetc(f);
        if (c == EOF) {
            P2SC_Msg(LVL_FATAL_CORRUPT_INPUT_DATA, "%s: unexpected end of file",
                     name);
            return -1;
        }
    } while (c == ' ' || c == '\t' || c == '\n' || c == '\r');
    ungetc(c, f);

    return 0;
}

guint16 *swap_read_pgm(const char *name, size_t * w, size_t * h)
{
    FILE *f;
    int c, i, len, isize, width, height, maxval;

    guint16 *ret = NULL;

    if (!(f = fopen(name, "rb")))
        P2SC_Msg(LVL_FATAL_FILESYSTEM, "%s: %s", name, g_strerror(errno));

    c = fgetc(f);
    if (c != 'P')
        P2SC_Msg(LVL_FATAL_CORRUPT_INPUT_DATA, "%s: expected P, got %d", name,
                 c);
    c = fgetc(f);
    if (c != '5')
        P2SC_Msg(LVL_FATAL_CORRUPT_INPUT_DATA, "%s: expected 5, got %d", name,
                 c);
    blank(f, name);
    if (fscanf(f, "%d", &width) != 1)
        P2SC_Msg(LVL_FATAL_CORRUPT_INPUT_DATA, "%s: failed read of width",
                 name);
    blank(f, name);
    if (fscanf(f, "%d", &height) != 1)
        P2SC_Msg(LVL_FATAL_CORRUPT_INPUT_DATA, "%s: failed read of height",
                 name);
    blank(f, name);
    if (fscanf(f, "%d", &maxval) != 1)
        P2SC_Msg(LVL_FATAL_CORRUPT_INPUT_DATA, "%s: failed read of maxval",
                 name);
    blank(f, name);

    if (width <= 0 || height <= 0 || maxval <= 0 ||
        width > 32767 || height > 32767 || maxval > 65535)
        P2SC_Msg(LVL_FATAL_CORRUPT_INPUT_DATA,
                 "%s: invalid image - width = %d, height = %d, maxval = %d",
                 name, width, height, maxval);

    len = width * height;
    ret = (guint16 *) g_malloc(len * sizeof *ret);

    if (maxval < 256)
        isize = 1;
    else
        isize = 2;

    if (fread(ret, len * isize, 1, f) != 1)
        P2SC_Msg(LVL_WARNING_CORRUPT_INPUT_DATA, "fread(%s): failed", name);
    fclose(f);

    if (isize == 2)
        for (i = 0; i < len; ++i)
            ret[i] = GUINT16_FROM_BE(ret[i]);
    else
        for (i = len - 1; i >= 0; --i)
            ret[i] = ((guint8 *) ret)[i];

    *w = width;
    *h = height;

    return ret;
}

void swap_y4m(const char *name, const char *cm, const guint8 * in, size_t w,
              size_t h)
{
    FILE *f;
    size_t l = w * h;

    swap_image_yuv_t *im = swap_mono2yuv(cm, in, w, h);
    swap_yuv2yuv420(im);

    if (access(name, F_OK)) {
        char *head;

        if (!(f = fopen(name, "wb")))
            P2SC_Msg(LVL_FATAL_FILESYSTEM, "%s: %s", name, g_strerror(errno));
        head =
            g_strdup_printf("YUV4MPEG2 W%zd H%zd F10:1 Ip A1:1 C420\n", w, h);
        fwrite(head, strlen(head), 1, f);
        g_free(head);
    } else {
        if (!(f = fopen(name, "ab")))
            P2SC_Msg(LVL_FATAL_FILESYSTEM, "%s: %s", name, g_strerror(errno));
    }

    fwrite("FRAME\n", 6, 1, f);
    if (fwrite(im->y, 1, l, f) != l)
        P2SC_Msg(LVL_FATAL_FILESYSTEM, "Y write error");
    if (fwrite(im->u, 1, l / 4, f) != l / 4)
        P2SC_Msg(LVL_FATAL_FILESYSTEM, "U write error");
    if (fwrite(im->v, 1, l / 4, f) != l / 4)
        P2SC_Msg(LVL_FATAL_FILESYSTEM, "V write error");

    fclose(f);

    swap_image_yuv_free(im);
}
