/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

static const char _versionid_[] __attribute__ ((unused)) =
    "$Id: p2sc_zdb.c 5108 2014-06-19 12:29:23Z bogdan $";

#include <string.h>
#include <glib.h>

#include "zdb/URL.h"
#include "zdb/ResultSet.h"
#include "zdb/PreparedStatement.h"
#include "zdb/Connection.h"
#include "zdb/ConnectionPool.h"
#include "zdb/Exception.h"

#include "p2sc_zdb.h"

struct zdb_connection_t {
    URL_T url;
    ConnectionPool_T pool;
    Connection_T con;
};

zdb_connection_t *zdb_connection_new(const char *uri)
{
    zdb_connection_t *c = (zdb_connection_t *) g_malloc(sizeof *c);

    c->url = URL_new(uri);
    c->pool = ConnectionPool_new(c->url);
    ConnectionPool_start(c->pool);

    c->con = ConnectionPool_getConnection(c->pool);
    Connection_setQueryTimeout(c->con, 30000);

    return c;
}

void zdb_connection_free(zdb_connection_t * c)
{
    if (c) {
        Connection_close(c->con);
        ConnectionPool_free(&c->pool);
        URL_free(&c->url);

        memset(c, 0, sizeof *c);
        g_free(c);
    }
}

void *zdb_connection_get(zdb_connection_t * c)
{
    if (c)
        return c->con;
    else
        return NULL;
}
