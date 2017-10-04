/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2012 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

static const char _versionid_[] __attribute__ ((unused)) =
    "$Id: opj_extra.c 5107 2014-06-19 12:26:29Z bogdan $";

#include <string.h>
#include <stdio.h>

#include "openjpeg.h"
#include "cio.h"
#include "j2k.h"
#include "jp2.h"

/* minimize intrusion */

#ifndef JP2_XML
#define JP2_XML 0x786d6c20
#endif

typedef struct {
    const char *xml;
    const unsigned char (*map)[256][3];
} swap_client_t;

void jp2_write_xml(opj_jp2_t * jp2, opj_cio_t * cio)
{
    swap_client_t *client = (swap_client_t *) jp2->cinfo->client_data;
    const char *xml;

    if (client && (xml = client->xml)) {
        size_t i, len = strlen(xml);
        opj_jp2_box_t box;

        box.init_pos = cio_tell(cio);
        cio_skip(cio, 4);
        cio_write(cio, JP2_XML, 4); /* XML */

        for (i = 0; i < len; ++i)
            cio_write(cio, xml[i], 1);

        box.length = cio_tell(cio) - box.init_pos;
        cio_seek(cio, box.init_pos);
        cio_write(cio, box.length, 4);  /* L */
        cio_seek(cio, box.init_pos + box.length);
    }
}

static void jp2_write_colr_real(opj_jp2_t * jp2, opj_cio_t * cio);
static void jp2_write_pclr(opj_jp2_t * jp2, opj_cio_t * cio);
static void jp2_write_cmap(opj_jp2_t * jp2, opj_cio_t * cio);

void jp2_write_colr(opj_jp2_t * jp2, opj_cio_t * cio)
{
    jp2_write_colr_real(jp2, cio);
    jp2_write_pclr(jp2, cio);
    jp2_write_cmap(jp2, cio);
}

static void jp2_write_colr_real(opj_jp2_t * jp2, opj_cio_t * cio)
{
    opj_jp2_box_t box;

    box.init_pos = cio_tell(cio);
    cio_skip(cio, 4);
    cio_write(cio, JP2_COLR, 4);    /* COLR */

    jp2->meth = 1;
    jp2->precedence = 0;
    jp2->approx = 0;

    cio_write(cio, jp2->meth, 1);   // METH
    cio_write(cio, jp2->precedence, 1); // PRECEDENCE
    cio_write(cio, jp2->approx, 1); // APPROX

    if (jp2->meth == 2)
        jp2->enumcs = 0;

    jp2->enumcs = 16;
    cio_write(cio, jp2->enumcs, 4); // EnumCS

    box.length = cio_tell(cio) - box.init_pos;
    cio_seek(cio, box.init_pos);
    cio_write(cio, box.length, 4);  /* L */
    cio_seek(cio, box.init_pos + box.length);
}

static void jp2_write_pclr(opj_jp2_t * jp2, opj_cio_t * cio)
{
    opj_jp2_box_t box;

    box.init_pos = cio_tell(cio);
    cio_skip(cio, 4);
    cio_write(cio, JP2_PCLR, 4);    /* PCLR */

    cio_write(cio, 256, 2);     // NE
    cio_write(cio, 3, 1);       // NPC

    cio_write(cio, 7, 1);       // Bi
    cio_write(cio, 7, 1);
    cio_write(cio, 7, 1);

    swap_client_t *client = (swap_client_t *) jp2->cinfo->client_data;
    const unsigned char (*map)[256][3];

    if (client && (map = client->map)) {
        for (int i = 0; i < 256; ++i) {
            cio_write(cio, (*map)[i][0], 1);
            cio_write(cio, (*map)[i][1], 1);
            cio_write(cio, (*map)[i][2], 1);
        }
    }

    box.length = cio_tell(cio) - box.init_pos;
    cio_seek(cio, box.init_pos);
    cio_write(cio, box.length, 4);  /* L */
    cio_seek(cio, box.init_pos + box.length);
}

static void jp2_write_cmap(opj_jp2_t * jp2, opj_cio_t * cio)
{
    opj_jp2_box_t box;

    box.init_pos = cio_tell(cio);
    cio_skip(cio, 4);
    cio_write(cio, JP2_CMAP, 4);    /* CMAP */

    cio_write(cio, 0, 2);       // CMP
    cio_write(cio, 1, 1);       // MTYP
    cio_write(cio, 0, 1);       // PCOL

    cio_write(cio, 0, 2);       // CMP
    cio_write(cio, 1, 1);       // MTYP
    cio_write(cio, 1, 1);       // PCOL

    cio_write(cio, 0, 2);       // CMP
    cio_write(cio, 1, 1);       // MTYP
    cio_write(cio, 2, 1);       // PCOL

    box.length = cio_tell(cio) - box.init_pos;
    cio_seek(cio, box.init_pos);
    cio_write(cio, box.length, 4);  /* L */
    cio_seek(cio, box.init_pos + box.length);
}
