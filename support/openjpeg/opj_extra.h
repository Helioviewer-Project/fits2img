/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2012 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

#ifndef __OPJ_EXTRA_H__
#define __OPJ_EXTRA_H__

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------- */

    void jp2_write_xml(opj_jp2_t *, opj_cio_t *);
    void jp2_write_colr(opj_jp2_t *, opj_cio_t *);

/* ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif
#endif
