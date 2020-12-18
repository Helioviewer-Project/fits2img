/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

#ifndef __P2SC_XML_H__
#define __P2SC_XML_H__

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------- */

#include "genx.h"

#define GENX_Try(w, stmt) \
do { \
    if (stmt) \
        P2SC_Msg(LVL_FATAL_INTERNAL_ERROR, "Genx: %s", \
                 genxLastErrorMessage(w)); \
} while (0)

    genxWriter p2sc_xml_start(p2sc_buffer_t *);
    void p2sc_xml_end(genxWriter);

    void p2sc_xml_addtext(genxWriter, const char *, ...)
        __attribute__((format(printf, 2, 3)));

    void p2sc_xml_element(genxWriter, const char *, const char *, ...)
        __attribute__((format(printf, 3, 4)));

/* ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif
#endif
