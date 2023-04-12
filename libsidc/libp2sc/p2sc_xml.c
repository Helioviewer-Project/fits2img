/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

static const char _versionid_[] __attribute__((unused)) =
    "$Id: p2sc_xml.c 5108 2014-06-19 12:29:23Z bogdan $";

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <glib.h>
#include <glib/gprintf.h>

#include "p2sc_buffer.h"
#include "p2sc_msg.h"
#include "p2sc_xml.h"

static void genx_addText(genxWriter w, const char *fmt, va_list args) {
    gint len;
    gchar *text;
    genxStatus status;

    len = g_vasprintf(&text, fmt, args);
    status = genxAddText(w, (constUtf8) text);

    if (status == GENX_BAD_UTF8 || status == GENX_NON_XML_CHARACTER) {
        utf8 ntext = (utf8) g_malloc(len + 1);

        genxScrubText(w, (constUtf8) text, ntext);
        status = genxAddText(w, ntext);
        g_free(ntext);
    }
    g_free(text);

    GENX_Try(w, status);
}

static void genx_addAttribute(genxWriter w, const char *name, const char *value) {
    genxStatus status = genxAddAttributeLiteral(w, NULL, (constUtf8) name, (constUtf8) value);
    if (status == GENX_BAD_UTF8 || status == GENX_NON_XML_CHARACTER) {
        utf8 nvalue = (utf8) g_malloc(strlen(value) + 1);

        genxScrubText(w, (constUtf8) value, nvalue);
        status = genxAddAttributeLiteral(w, NULL, (constUtf8) name, nvalue);
        g_free(nvalue);
    }
    GENX_Try(w, status);
}

void p2sc_xml_addtext(genxWriter w, const char *fmt, ...) {
    va_list va;

    va_start(va, fmt);
    genx_addText(w, fmt, va);
    va_end(va);
}

void p2sc_xml_element(genxWriter w, const char *name, const char *attr, const char *attr_value,
                      const char *fmt, ...) {
    va_list va;

    GENX_Try(w, genxStartElementLiteral(w, NULL, (constUtf8) name));
    if (attr)
        genx_addAttribute(w, attr, attr_value);

    va_start(va, fmt);
    genx_addText(w, fmt, va);
    va_end(va);

    GENX_Try(w, genxEndElement(w));
}

static genxStatus _xsend(void *u, constUtf8 s) {
    p2sc_buffer_write((p2sc_buffer_t *) u, strlen((const char *) s), s);
    return GENX_SUCCESS;
}

static genxStatus _xsendbounded(void *u, constUtf8 start, constUtf8 end) {
    if (end < start)
        return GENX_INTERNAL_ERROR;

    p2sc_buffer_write((p2sc_buffer_t *) u, end - start, start);
    return GENX_SUCCESS;
}

static genxStatus _xflush(void *u G_GNUC_UNUSED) {
    return GENX_SUCCESS;
}

static genxSender bufSender = { _xsend, _xsendbounded, _xflush };

genxWriter p2sc_xml_start(p2sc_buffer_t *buf) {
    genxStatus status;
    genxWriter w = genxNew(NULL, NULL, buf);

    /* if buf is NULL, write to stdout */
    if (buf)
        status = genxStartDocSender(w, &bufSender);
    else
        status = genxStartDocFile(w, stdout);

    GENX_Try(w, status);

    return w;
}

void p2sc_xml_end(genxWriter w) {
    GENX_Try(w, genxEndDocument(w));
    genxDispose(w);
}
