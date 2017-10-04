/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

#ifndef __SWAP_META_H__
#define __SWAP_META_H__

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------- */

    char *swap_fits2xml(sfts_t * f);
    char *swap_fits2hv(sfts_t *, const char *);

/* ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif
#endif
