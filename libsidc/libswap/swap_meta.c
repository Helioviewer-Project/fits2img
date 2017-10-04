/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

static const char _versionid_[] __attribute__ ((unused)) =
    "$Id: swap_meta.c 5110 2014-06-19 12:37:15Z bogdan $";

#include <string.h>
#include <glib.h>

#include "p2sc_buffer.h"
#include "p2sc_fits.h"
#include "p2sc_msg.h"
#include "p2sc_stdlib.h"
#include "p2sc_time.h"
#include "p2sc_xml.h"
#include "swap_meta.h"

static void head2xml(sfts_t * f, genxWriter w)
{
    GENX_Try(w, genxStartElementLiteral(w, NULL, (constUtf8) "fits"));

    char **keys, **vals, **typs;
    sfts_head2str(f, NULL, &keys, &vals, &typs);

    int i, n = g_strv_length(keys);
    for (i = 0; i < n; ++i) {
        if (!strcmp("VARCHAR(72)", typs[i])) {
            vals[i][0] = ' ';
            vals[i][strlen(vals[i]) - 1] = ' ';
            g_strstrip(vals[i]);
        }
        p2sc_xml_element(w, keys[i], "%s", vals[i]);
    }

    g_strfreev(keys), g_strfreev(vals), g_strfreev(typs);

    GENX_Try(w, genxEndElement(w)); /* fits */
}

char *swap_fits2xml(sfts_t * f)
{
    p2sc_buffer_t *b = p2sc_buffer_new(NULL, 4096);

    genxWriter w = p2sc_xml_start(b);
    head2xml(f, w);
    p2sc_xml_end(w);

    const guint8 zero = 0;
    p2sc_buffer_write(b, 1, &zero);

    return (char *) p2sc_buffer_del(b, TRUE);
}

char *swap_fits2hv(sfts_t * f, const char *contact)
{
    p2sc_buffer_t *b = p2sc_buffer_new(NULL, 4096);
    genxWriter w = p2sc_xml_start(b);

    GENX_Try(w, genxStartElementLiteral(w, NULL, (constUtf8) "meta"));

    head2xml(f, w);

    GENX_Try(w, genxStartElementLiteral(w, NULL, (constUtf8) "helioviewer"));
    GENX_Try(w, genxStartElementLiteral(w, NULL, (constUtf8) "HV_COMMENT"));
    {
        char *s_time = p2sc_timestamp(-1, 3);
        char *s_soft =
            g_strdup_printf("%s - %s (%s)", p2sc_get_string("appname"),
                            p2sc_get_string("prgname"), _versionid_);

        p2sc_xml_addtext(w, "\n"
                         " Title         : %s\n"
                         " Author        : ROB\n"
                         " Contact       : %s\n"
                         " Copyright     : Public Domain\n"
                         " Creation Time : %s\n"
                         " Software      : %s\n"
                         " Source        : %s\n",
                         p2sc_get_string("filename"), contact, s_time, s_soft,
                         g_get_host_name());
        g_free(s_time), g_free(s_soft);
    }
    GENX_Try(w, genxEndElement(w)); /* HV_COMMENT */
    GENX_Try(w, genxEndElement(w)); /* helioviewer */

    GENX_Try(w, genxEndElement(w)); /* meta */
    p2sc_xml_end(w);

    const guint8 zero = 0;
    p2sc_buffer_write(b, 1, &zero);

    return (char *) p2sc_buffer_del(b, TRUE);
}
