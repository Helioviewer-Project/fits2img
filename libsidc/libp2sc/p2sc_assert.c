/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

static const char _versionid_[] __attribute__ ((unused)) =
    "$Id: p2sc_assert.c 5108 2014-06-19 12:29:23Z bogdan $";

#include "zdb/Exception.h"

#include "p2sc_msg.h"
#include "p2sc_assert.h"

/* ---------------------------------------------------------------------- */

extern void (*AbortHandler) (const char *error);

static void _exception_init_(void) __attribute__ ((constructor));

static void _abort_handler_(const char *msg)
{
    P2SC_Msg(LVL_FATAL, "%s", msg);
}

static void _exception_init_(void)
{
    AbortHandler = _abort_handler_;
    Exception_init();
}

/* ---------------------------------------------------------------------- */
