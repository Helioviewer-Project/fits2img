/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

#ifndef __P2SC_BUFFER_H__
#define __P2SC_BUFFER_H__

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------- */

    typedef struct p2sc_buffer_t p2sc_buffer_t;

    p2sc_buffer_t *p2sc_buffer_new(guint8 *, size_t);
    guint8 *p2sc_buffer_del(p2sc_buffer_t *, gboolean);

    const guint8 *p2sc_buffer_read(p2sc_buffer_t *, size_t);
    void p2sc_buffer_write(p2sc_buffer_t *, size_t, const guint8 *);
    void p2sc_buffer_rewind(p2sc_buffer_t *);

    size_t p2sc_buffer_size(p2sc_buffer_t *);
    size_t p2sc_buffer_position(p2sc_buffer_t *);
    size_t p2sc_buffer_remaining(p2sc_buffer_t *);

/* ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif
#endif
