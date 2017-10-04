/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

static const char _versionid_[] __attribute__ ((unused)) =
    "$Id: send2LMAT.c 5113 2014-06-19 15:07:34Z bogdan $";

#include <unistd.h>
#include <stdio.h>
#include <glib.h>

#include "send2LMAT.h"

static gboolean _s2err G_GNUC_UNUSED = FALSE;
static gboolean _s2dir G_GNUC_UNUSED = FALSE;

#ifndef STANDALONE

#include <signal.h>
#include <string.h>

/* ---------------------------------------------------------------------- */

static void _s2l_init_(void) __attribute__ ((constructor));

static void _s2l_init_(void)
{
    if (g_getenv("P2SC_SEND2STDERR"))
        _s2err = TRUE;
    if (g_getenv("P2SC_SEND2DIR"))
        _s2dir = TRUE;
}

/* ---------------------------------------------------------------------- */

static gboolean lmat_spawn(gchar ** argv, gboolean synchronous)
{
    gboolean ret;
    GError *err = NULL;
    GSpawnFlags flags = (GSpawnFlags) 0;

    if (synchronous)
        ret = g_spawn_sync(NULL, argv, NULL, flags, NULL, NULL, NULL, NULL,
                           NULL, &err);
    else {
#ifdef G_OS_UNIX
        struct sigaction sa;

        memset(&sa, 0, sizeof sa);
        sa.sa_handler = SIG_IGN;

        sigaction(SIGCHLD, &sa, NULL);
#endif

        flags = (GSpawnFlags) G_SPAWN_DO_NOT_REAP_CHILD;
        ret = g_spawn_async(NULL, argv, NULL, flags, NULL, NULL, NULL, &err);
    }

    if (!ret) {
        if (err && err->message) {
            fprintf(stderr, "%s: %s\n", __func__, err->message);
            g_error_free(err);
        } else
            fprintf(stderr, "%s: spawn failed - unknown reason\n", __func__);
    }

    return ret;
}

static gboolean p2sc_send2LMAT(char *nam, char *ver, char *rid, char *msg,
                               char *mid, char *loc, char *dbg, char *pid,
                               char *host)
{
    gboolean ret = FALSE;
    gchar *send2LMATscript =
        g_build_filename(SEND2LMAT_DIR, "send2LMAT.pl", NULL);

    if (g_file_test(send2LMATscript, G_FILE_TEST_IS_EXECUTABLE)) {
        gchar *argv[] = {
            send2LMATscript, nam, ver, rid, msg, mid, loc, dbg, pid, host, NULL
        };

        ret = lmat_spawn(argv, TRUE);
    } else
        fprintf(stderr,
                "%s: does not exist or is not executable, using stderr:\n",
                send2LMATscript);

    g_free(send2LMATscript);

    return ret;
}

#endif

void send2LMAT(const char *name, const char *version, int runid,
               const char *message, int msgid, const char *location,
               const char *debug_dir)
{
/*
    g_spawn_*'s argv arguments are of gchar ** type indicating that they may be
    modified by the called function
*/
    gchar *nam = name ? g_strdup(name) : g_strdup("NULL");
    gchar *ver = version ? g_strdup(version) : g_strdup("NULL");
    gchar *msg = message ? g_strdup(message) : g_strdup("NULL");
    gchar *loc = location ? g_strdup(location) : g_strdup("NULL");
    gchar *dbg G_GNUC_UNUSED =
        _s2dir && debug_dir ? g_strdup(debug_dir) : g_strdup("NULL");

    gchar *rid = g_strdup_printf("%d", runid);
    gchar *mid = g_strdup_printf("%d", msgid);
    gchar *pid = g_strdup_printf("%ld", (long) getpid());

    gchar *host = g_strdup(g_get_host_name());

#ifndef STANDALONE
    if (!_s2err &&
        p2sc_send2LMAT(nam, ver, rid, msg, mid, loc, dbg, pid, host)) ;
    else
#endif
        fprintf(stderr, "%s [PID %s RUNID %s] on %s:\n\t%s: %s\n[%s - %s]\n",
                nam, pid, rid, host, mid, msg, loc, ver);

    g_free(host);

    g_free(pid);
    g_free(mid);
    g_free(rid);

    g_free(dbg);
    g_free(loc);
    g_free(msg);
    g_free(ver);
    g_free(nam);
}
