/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

static const char _versionid_[] __attribute__ ((unused)) =
	"$Id: sidc_log.c 479 2014-02-03 12:55:31Z bogdan $";

#include <unistd.h>
#include <stdio.h>
#include <glib.h>

#include "sidc_log.h"

#define SIDC_MSG_LOG "(" MSG_HEAD MSG_LOG ")"

typedef struct {
	guint64 rid;
	gint64 pid;
	gint32 mid;

	char *host;
	char *name;
	char *vers;
	char *mesg;
	char *locn;
} sidc_log_t;

GVariant *sidc_log(const char *name, const char *version, guint64 runid,
				   const char *message, gint32 msgid, const char *location,
				   const char *debug_dir G_GNUC_UNUSED)
{
	return
		g_variant_new(SIDC_MSG_LOG,
					  (guint32) MSG_LOG_TYPE, runid, (gint64) getpid(), msgid,
					  g_get_host_name(), name, version, message, location);
}

void sidc_unlog(const char *p, size_t s)
{
	GVariant *v =
		g_variant_new_from_data((const GVariantType *) "(u(txismsmsmsms))",
								p, s, FALSE, NULL, NULL);

/*	{
		GVariant *norm = g_variant_get_normal_form(v);
		g_variant_unref(v);
		v = norm;
	}
*/
	if (G_BYTE_ORDER == G_BIG_ENDIAN) {
		GVariant *vswap = g_variant_byteswap(v);
		g_variant_unref(v);
		v = vswap;
	}

	guint32 t;
	g_variant_get_child(v, 0, "u", &t);
	if (t != MSG_LOG_TYPE) {
		g_variant_unref(v);
		return;
	}

	sidc_log_t l;
	g_variant_get_child(v, 1, MSG_LOG, &l.rid, &l.mid, &l.pid, &l.host, &l.name,
						&l.vers, &l.mesg, &l.locn);

	g_variant_unref(v);
}
