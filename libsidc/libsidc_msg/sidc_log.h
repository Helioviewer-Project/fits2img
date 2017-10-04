/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

#ifndef __SIDC_LOG_H__
#define __SIDC_LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------- */

#define MSG_HEAD "u"
#define MSG_LOG "(txi&sm&sm&sm&sm&s)"
#define MSG_LOG_TYPE 1000

	GVariant *sidc_log(const char *name, const char *version, guint64 runid,
					   const char *message, gint32 msgid, const char *location,
					   const char *debug_dir G_GNUC_UNUSED);
	void sidc_unlog(const char *p, size_t s);

/* ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif
#endif
