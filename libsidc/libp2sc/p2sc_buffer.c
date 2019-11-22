/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

static const char _versionid_[] __attribute__ ((unused)) =
    "$Id: p2sc_buffer.c 5108 2014-06-19 12:29:23Z bogdan $";

#include <string.h>
#include <glib.h>

#include "p2sc_buffer.h"
#include "p2sc_msg.h"

#define SIZBUF 0xffff

struct p2sc_buffer_t {
    guint8 *ptr;
    size_t allen;
    size_t len;
    size_t pos;

    gboolean own;
};

p2sc_buffer_t *p2sc_buffer_new(guint8 * ptr, size_t len)
{
    p2sc_buffer_t *buf = (p2sc_buffer_t *) g_malloc(sizeof *buf);

    buf->pos = 0;

    if (ptr) {
        buf->ptr = ptr;
        buf->allen = len;
        buf->len = len;
        buf->own = FALSE;
    } else {
        buf->ptr = (guint8 *) g_malloc(SIZBUF);
        buf->allen = SIZBUF;
        buf->len = 0;
        buf->own = TRUE;
    }

    return buf;
}

guint8 *p2sc_buffer_del(p2sc_buffer_t * buf, gboolean preserve)
{
    guint8 *ret = NULL;

    if (buf->own) {
        if (preserve)
            ret = (guint8 *) g_realloc(buf->ptr, buf->len);
        else
            g_free(buf->ptr);
    } else
        ret = buf->ptr;

    memset(buf, 0, sizeof *buf);
    g_free(buf);
    return ret;
}

const guint8 *p2sc_buffer_read(p2sc_buffer_t * buf, size_t size)
{
    guint8 *ret = buf->ptr + buf->pos;

    if (buf->pos + size > buf->len)
        return NULL;

    buf->pos += size;
    return ret;
}

void p2sc_buffer_write(p2sc_buffer_t * buf, size_t size, const guint8 * ptr)
{
    size_t new_pos = buf->pos + size;

    if (new_pos > buf->allen) {
        if (!buf->own)
            P2SC_Msg(LVL_FATAL_INTERNAL_ERROR, "Buffer out of bounds");
        buf->allen = MAX(new_pos, 2 * buf->allen);
        buf->ptr = (guint8 *) g_realloc(buf->ptr, buf->allen);
    }
    memcpy(buf->ptr + buf->pos, ptr, size);
    buf->pos = new_pos;
    buf->len += size;
}

void p2sc_buffer_rewind(p2sc_buffer_t * buf)
{
    buf->pos = 0;
}

size_t p2sc_buffer_size(p2sc_buffer_t * buf)
{
    return buf->len;
}

size_t p2sc_buffer_position(p2sc_buffer_t * buf)
{
    return buf->pos;
}

size_t p2sc_buffer_remaining(p2sc_buffer_t * buf)
{
    return buf->len - buf->pos;
}
