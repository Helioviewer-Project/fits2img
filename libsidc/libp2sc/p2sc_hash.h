/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

#ifndef __P2SC_HASH_H__
#define __P2SC_HASH_H__

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------- */

    extern const guint16 crc16table[256];
    extern const guint32 crc32table[256];

    /* initialise crc16 = 0xffff */
    static inline guint16 p2sc_crc16(guint16 crc, const guint8 * data, size_t len) {
        while (len--)
            crc = (crc << 8) ^ crc16table[(crc >> 8) ^ *data++];
        return crc;
    }
    /* initialise crc32 = 0xffffffff */
        static inline guint32 p2sc_crc32(guint32 crc, const guint8 * data,
                                         size_t len) {
        while (len--)
            crc = (crc << 8) ^ crc32table[(crc >> 24) ^ *data++];
        return crc;
    }

    /* needed for results identical to cksum, also initialise with 0 */
    static inline guint32 p2sc_crc32_finalise(guint32 crc, size_t len) {
        while (len) {
            guint8 clen = len & 0xffU;

            crc = p2sc_crc32(crc, &clen, 1);
            len >>= 8;
        }

        return ~crc;
    }

/* ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif
#endif
