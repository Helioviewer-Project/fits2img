/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

#ifndef __P2SC_ASSERT_H__
#define __P2SC_ASSERT_H__

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------- */

#include "zdb/Exception.h"

    extern Exception_T AssertException;

#ifdef ASSERT
#undef ASSERT
#endif

/* #define ASSERT(e) ((void)((e)||(Exception_throw(&(AssertException), __func__, __FILE__, __LINE__, #e),0))) */
#define ASSERT(e, ...) if (!(e)) Exception_throw(&(AssertException), __func__, __FILE__, __LINE__, __VA_ARGS__);
#define ASSERT0(e) ASSERT(e, #e);

/* ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif
#endif
