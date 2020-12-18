/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

static const char _versionid_[] __attribute__((unused)) =
    "$Id: p2sc_msg.c 5108 2014-06-19 12:29:23Z bogdan $";

#include <stdarg.h>
#include <stdlib.h>

#include <glib.h>
#include <glib/gprintf.h>

#include "send2LMAT.h"
#include "p2sc_stdlib.h"
#include "p2sc_msg.h"

static char *p2sc_backtrace(void);

void _p2sc_msg(const char *func, const char *file, int line,
               const char *versionid, int severity, const char *fmt, ...) {
    va_list va;
    gint len;
    char *msg, *loc, *back;
    const char *proc_file = p2sc_get_string("filename");

    /* format message */
    va_start(va, fmt);
    len = g_vasprintf(&msg, fmt, va);
    va_end(va);

    if (msg[len - 1] == '\n')
        msg[len - 1] = 0;

    /* append to history */
    const char *hist = p2sc_get_string("history");
    if (hist) {
        char *nhist = g_strdup_printf("%s|%s", hist, msg);

        p2sc_set_string("history", nhist);
        g_free(nhist);
    } else
        p2sc_set_string("history", msg);

    /* add processed file if any */
    if (proc_file) {
        char *msg2 = g_strdup_printf("%s - %s", proc_file, msg);

        g_free(msg);
        msg = msg2;
    }

    /* add location */
    loc = g_strdup_printf("%s - %s() in %s:%d", p2sc_get_string("prgname"), func, file, line);
    /* add backtrace */
    if (severity >= LVL_WARNING && (back = p2sc_backtrace())) {
        if (*back) {
            char *loc2 = g_strdup_printf("%s%s\n", loc, back);

            g_free(loc);
            loc = loc2;
        }
        g_free(back);
    }

    gboolean fatal_in_ppt = FALSE;
    /* check if we backtrace from PPT - ugly hack */
    if (severity >= LVL_FATAL && (g_strstr_len(loc, -1, "ppt.so") ||
                                  g_strstr_len(loc, -1, "libppt."))) {
        severity = LVL_FATAL_PPT;
        fatal_in_ppt = TRUE;
    }

    /* finally send the message */
    send2LMAT(p2sc_get_string("appname"), versionid, p2sc_get_string("runid"), msg,
              severity, loc, NULL);

    /* send additional message if killed from PPT */
    if (fatal_in_ppt)
        send2LMAT(p2sc_get_string("appname"), NULL, p2sc_get_string("runid"),
                  "Terminated by PPT", LVL_CONTROL_FINISH_STOP_PPT, NULL, NULL);

    g_free(loc);
    g_free(msg);
}

#ifdef __APPLE__

#include <AvailabilityMacros.h>
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5)
#define HAVE_BACKTRACE
#include <execinfo.h>
#endif

#else

#ifdef __GLIBC__

#if (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 1) || (__GLIBC__ > 2)
#define HAVE_BACKTRACE
#include <execinfo.h>
#endif

#endif

#endif

static char *p2sc_backtrace(void) {
#ifdef HAVE_BACKTRACE

#define BT_DEPTH 128

    void *stack[BT_DEPTH + 1];
    int frames = backtrace(stack, BT_DEPTH);
    char **strs = backtrace_symbols(stack, frames), *ret;

    strs[frames] = NULL;
    ret = g_strjoinv("\n\t", strs);
    free(strs);

    return ret;
#endif
    return NULL;
}
