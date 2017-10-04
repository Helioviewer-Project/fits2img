/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

#ifndef __SWAP_FILE_H__
#define __SWAP_FILE_H__

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------- */

    void swap_write_png(const char *, const guint8 *, size_t, size_t,
                        swap_palette_t *);
    void swap_write_jpg(const char *, const guint8 *, size_t, size_t,
                        swap_palette_t *, int);

    void swap_write_pgm(const char *, const guint16 *, size_t, size_t, guint16);
    guint16 *swap_read_pgm(const char *, size_t *, size_t *);

    void swap_y4m(const char *, const char *, const guint8 *, size_t, size_t);

/* ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif
#endif
