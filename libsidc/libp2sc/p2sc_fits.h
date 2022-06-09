/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

#ifndef __P2SC_FITS_H__
#define __P2SC_FITS_H__

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------- */

#define SKEY_LEN 81

#define SUINT8  (1 << 0)
#define SUINT16 (1 << 1)
#define SUINT32 (1 << 2)
#define SUINT64 (1 << 3)

#define SINT8   (1 << 4)
#define SINT16  (1 << 5)
#define SINT32  (1 << 6)
#define SINT64  (1 << 7)

#define SFLOAT  (1 << 8)
#define SDOUBLE (1 << 9)

#define SFTS_SUM_NOVERIFY ((int) 0xdeadbeef)

    typedef struct sfts_t sfts_t;

    typedef struct {
        const char *k;

        char t;
        union {
            double f;
            gint64 i;
            guint8 b;
            char *s;
        } v;

        const char *c;
    } sfkey_t;

    sfts_t *sfts_create(const char *, const char *);
    sfts_t *sfts_openro(const char *, ...);
    char *sfts_free(sfts_t *);

    int sfts_get_nhdus(sfts_t *);
    void sfts_goto_hdu(sfts_t *, int);

    void sfts_copy(sfts_t *, sfts_t *);
    void sfts_copy_header(sfts_t *, sfts_t *);

    void *sfts_read_image(sfts_t *, size_t *, size_t *, int);
    void sfts_create_image(sfts_t *, size_t, size_t, int);
    void sfts_write_image(sfts_t *, const void *, size_t, size_t, int);

    void sfts_find_hdukey(sfts_t *, const char *);

    int sfts_read_key(sfts_t *, sfkey_t *);
    int sfts_read_keymaybe(sfts_t *, sfkey_t *);
    char *sfts_read_keystring(sfts_t *, const char *);
    char *sfts_read_keystring0(sfts_t *, const char *);
    void sfts_write_key(sfts_t *, const sfkey_t *);
    void sfts_delete_key(sfts_t *, const char *);
    void sfts_write_comment(sfts_t *, const char *);

    char *sfts_read_history(sfts_t *);

    void sfts_head2str(sfts_t *, char ***, char ***, char ***);

    char *sfts_timestamp(char *);

/* ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif
#endif
