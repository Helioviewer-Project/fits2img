/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

static const char _versionid_[] __attribute__((unused)) =
    "$Id: swap_file_j2k.c 5110 2014-06-19 12:37:15Z bogdan $";

#include <stdio.h>
#include <string.h>
#include <glib.h>

#include "openjpeg.h"
#include "opj_index.h"

#include "p2sc_file.h"
#include "swap_color.h"
#include "swap_file_j2k.h"

#define J2_DEBUG  0
#define OPJ_INDEX "opj_index.txt"

#define J2K_CFMT  0
#define JP2_CFMT  1
#define JPT_CFMT  2

static void write_data(const char *, const guint8 *, size_t);

static void error_cb(const char *msg, void *client_data) {
    FILE *stream = (FILE *) client_data;
    fprintf(stream, "[ERROR] %s", msg);
}

static void warn_cb(const char *msg, void *client_data) {
    FILE *stream = (FILE *) client_data;
    fprintf(stream, "[WARNING] %s", msg);
}

static void info_cb(const char *msg, void *client_data) {
    FILE *stream = (FILE *) client_data;
    fprintf(stream, "[INFO] %s", msg);
}

guint8 *swap_read_j2k(const char *name, size_t *ww, size_t *hh, size_t *nc) {
    guint8 *ret = NULL, *r;
    opj_event_mgr_t event_mgr;

    memset(&event_mgr, 0, sizeof event_mgr);
    event_mgr.client_data = stderr;
    event_mgr.error_handler = error_cb;
    event_mgr.warning_handler = warn_cb;
    event_mgr.info_handler = info_cb;

    opj_initialize_default_event_handler(&event_mgr, TRUE);

    opj_dparameters_t params;
    opj_set_default_decoder_parameters(&params);
    opj_dinfo_t *dinfo = opj_create_decompress(CODEC_JP2);

    opj_setup_decoder(dinfo, &params);

    GMappedFile *map = p2sc_map_file(name);
    opj_cio_t *cio = opj_cio_open((opj_common_ptr) dinfo,
                                  (unsigned char *)
                                  g_mapped_file_get_contents(map),
                                  g_mapped_file_get_length(map));

    opj_codestream_info_t cstr_info;
    opj_image_t *image = opj_decode_with_info(dinfo, cio, &cstr_info);

    int ncomps = image->numcomps, i;
    if (ncomps == 1 ||
        (ncomps == 3
         && image->comps[0].dx == image->comps[1].dx
         && image->comps[1].dx == image->comps[2].dx
         && image->comps[0].dy == image->comps[1].dy
         && image->comps[1].dy == image->comps[2].dy
         && image->comps[0].sgnd == image->comps[1].sgnd
         && image->comps[1].sgnd == image->comps[2].sgnd
         && image->comps[2].sgnd == 0
         && image->comps[0].prec == image->comps[1].prec
         && image->comps[1].prec == image->comps[2].prec && image->comps[2].prec == 8)) {

        int w = image->comps[0].w, h = image->comps[0].h, l = w * h;
        ret = (guint8 *) g_malloc(l * ncomps), r = ret;

        if (ncomps == 1) {
            for (i = 0; i < l; ++i)
                ret[i] = image->comps[0].data[i];
        } else
            for (i = 0; i < l; ++i) {
                r[0] = image->comps[0].data[i];
                r[1] = image->comps[1].data[i];
                r[2] = image->comps[2].data[i];
                r += 3;
            }
        *ww = w, *hh = h, *nc = ncomps;
    }
#if J2_DEBUG
    write_index_file(&cstr_info, OPJ_INDEX);
#endif

    opj_destroy_cstr_info(&cstr_info);
    opj_image_destroy(image);
    opj_cio_close(cio);
    g_mapped_file_unref(map);
    opj_destroy_decompress(dinfo);

    return ret;
}

void swap_write_j2k(const char *name, const guint8 *in, size_t w, size_t h,
                    const swap_j2kparams_t *p) {
    int subsampling_dx = 1, subsampling_dy = 1;

    opj_event_mgr_t event_mgr;

    memset(&event_mgr, 0, sizeof event_mgr);
    event_mgr.client_data = stderr;
    event_mgr.error_handler = error_cb;
    event_mgr.warning_handler = warn_cb;
    event_mgr.info_handler = info_cb;

    opj_initialize_default_event_handler(&event_mgr, TRUE);

    opj_image_cmptparm_t comppar;

    memset(&comppar, 0, sizeof comppar);
    comppar.prec = 8;
    comppar.bpp = 8;
    comppar.sgnd = 0;
    comppar.dx = subsampling_dx;
    comppar.dy = subsampling_dy;
    comppar.w = w;
    comppar.h = h;

    opj_image_t *image = opj_image_create(1, &comppar, CLRSPC_GRAY);

    image->x0 = 0;
    image->y0 = 0;
    image->x1 = 0 + (w - 1) * subsampling_dx + 1;
    image->y1 = 0 + (h - 1) * subsampling_dy + 1;
    for (size_t i = 0; i < w * h; ++i)
        image->comps[0].data[i] = in[i];

    opj_cparameters_t params;
    opj_set_default_encoder_parameters(&params);

    params.cp_comment = g_strdup_printf("SIDC OpenJPEG v%s", opj_version());

    params.prog_order = RPCL;
    params.cod_format = JP2_CFMT;

    double cr = p->cratio < 1 ? 1 : p->cratio;
    params.tcp_numlayers = CLAMP(p->nlayers, 1, 32);
    params.tcp_rates[params.tcp_numlayers - 1] = cr;
    for (int i = params.tcp_numlayers - 2; i >= 0; --i)
        params.tcp_rates[i] = 2 * params.tcp_rates[i + 1];

    params.cp_disto_alloc = 1;
    params.irreversible = 1;

    params.numresolution = params.res_spec = CLAMP(p->nresolutions, 1, 32);
    for (int i = 0; i <= params.res_spec; ++i) {
        params.prcw_init[i] = p->precinct[0];
        params.prch_init[i] = p->precinct[1];
    }
    /* J2K_CP_CSTY_PRT - use precincts */
    if (p->precinct[0] > 1 && p->precinct[1] > 1)
        params.csty |= 0x01;

    /* get a JP2 compressor handle */
    opj_cinfo_t *cinfo = opj_create_compress(CODEC_JP2);
    /* setup the encoder parameters using the current image and using user parameters */
    opj_setup_encoder(cinfo, &params, image);
    cinfo->client_data = (void *) &p->meta;

    /* open a byte stream for writing */
    opj_cio_t *cio = opj_cio_open((opj_common_ptr) cinfo, NULL, 0);
    /* encode the image while constructing the codestream information */
    opj_codestream_info_t cstr_info;
    if (!opj_encode_with_info(cinfo, cio, image, &cstr_info)) {
        opj_cio_close(cio);
        fprintf(stderr, "failed to encode image\n");
        return;
    }

    write_data(name, cio->buffer, cio_tell(cio));

    if (p->debug) {
        char *idx = g_strdup_printf("%s.%s", name, OPJ_INDEX);
        write_index_file(&cstr_info, idx);
        g_free(idx);
    }

    opj_cio_close(cio);
    opj_destroy_cstr_info(&cstr_info);
    opj_destroy_compress(cinfo);
    opj_image_destroy(image);
    g_free(params.cp_comment);
}

static void write_data(const char *name, const guint8 *code, size_t code_len) {
    FILE *f = fopen(name, "wb");
    if (!f) {
        fprintf(stderr, "failed to open %s for writing\n", name);
        return;
    }

    if (fwrite(code, 1, code_len, f) < code_len) {  /* FIXME */
        fprintf(stderr, "failed to write %zd (%s)\n", code_len, name);
        return;
    }

    fclose(f);
}
