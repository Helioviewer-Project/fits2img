/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

#ifndef __P2SC_ZDB_H__
#define __P2SC_ZDB_H__

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------- */

#define ZDB_Begin \
    TRY {

#define ZDB_End \
    } ELSE { \
        char *exmsg = g_strdup_printf("%s: %s\n raised in %s at %s:%d", \
                                        Exception_frame.exception->name, \
                                        Exception_frame.message, \
                                        Exception_frame.func, \
                                        Exception_frame.file, \
                                        Exception_frame.line); \
        P2SC_Msg(LVL_FATAL, "libzdb: %s", exmsg); \
        g_free(exmsg); \
    } END_TRY

#define ZDB_Try(stmt) \
    do { \
        ZDB_Begin { \
            stmt; \
        } ZDB_End; \
    } while (0)

/* ---------------------------------------------------------------------- */

    typedef struct zdb_connection_t zdb_connection_t;

    zdb_connection_t *zdb_connection_new(const char *);
    void zdb_connection_free(zdb_connection_t *);

    void *zdb_connection_get(zdb_connection_t *);

/* ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif
#endif
