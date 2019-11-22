/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

static const char _versionid_[] __attribute__ ((unused)) =
    "$Id: p2sc_stdlib.c 5108 2014-06-19 12:29:23Z bogdan $";

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <glib.h>
#include <gmodule.h>

#include "p2sc_msg.h"
#include "p2sc_stdlib.h"

static int _runid_ = 0;
static GHashTable *shash = NULL;

/* ---------------------------------------------------------------------- */

static void _p2sc_init_(void) __attribute__ ((constructor));
static void _p2sc_fini_(void) __attribute__ ((destructor));

static inline void set_signals(sig_t handler)
{
    signal(SIGABRT, handler);
    signal(SIGILL, handler);
    signal(SIGSEGV, handler);
    signal(SIGFPE, handler);
    signal(SIGBUS, handler);
    signal(SIGPIPE, handler);
}

static void sig_handler(int sig)
{
    /* restore signals to avoid recursion */
    set_signals(SIG_DFL);
    P2SC_Msg(LVL_FATAL, "Signal %d caught: %s - terminating", sig,
             g_strsignal(sig));
}

static void glib_logger(const gchar * log_domain, GLogLevelFlags log_level,
                        const gchar * message, gpointer user_data G_GNUC_UNUSED)
{
    int level;

    if (log_level &
        (G_LOG_LEVEL_DEBUG | G_LOG_LEVEL_INFO | G_LOG_LEVEL_MESSAGE))
        level = LVL_MESSAGE;
    else if (log_level & G_LOG_LEVEL_WARNING)
        level = LVL_WARNING;
    else
        level = LVL_FATAL;

    P2SC_Msg(level, "%s: %s", log_domain, message);
}

static void _p2sc_init_(void)
{
    /* intercept signals */
    set_signals(sig_handler);
    /* redirect GLib messages to LMAT */
    g_log_set_handler("GLib", (GLogLevelFlags) ~ 0, glib_logger, NULL);

    shash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
}

static void _p2sc_fini_(void)
{
    if (shash) {
        g_hash_table_destroy(shash);
        shash = NULL;
    }
}

/* ---------------------------------------------------------------------- */

void p2sc_init(const char *prgname, const char *appname, const char *filename,
               int runid)
{
    /* change of prg/appname not allowed */
    if (prgname && !p2sc_get_string("prgname"))
        p2sc_set_string("prgname", prgname);
    if (appname && !p2sc_get_string("appname"))
        p2sc_set_string("appname", appname);
    /* allow change of filename, but avoid NULL */
    if (filename)
        p2sc_set_string("filename", filename);

    p2sc_set_runid(runid);

    char *hist =
        g_strdup_printf("%s %d", p2sc_get_string("appname"), p2sc_get_runid());
    p2sc_set_string("history", hist);
    g_free(hist);
}

void p2sc_set_runid(int runid)
{
    _runid_ = runid;
}

int p2sc_get_runid(void)
{
    return _runid_;
}

void p2sc_set_string(const char *key, const char *value)
{
    if (shash)
        g_hash_table_insert(shash, g_strdup(key), g_strdup(value));
}

const char *p2sc_get_string(const char *key)
{
    if (shash)
        return (const char *) g_hash_table_lookup(shash, key);
    else
        return NULL;
}

void p2sc_option_ext(int marg, int *argc, char ***argv, const char *appname,
                     const char *context, const char *summary,
                     const GOptionEntry * entries)
{
    gboolean ret;
    gint runid = 0;
    gchar *base_prog, *base_file = NULL;
    GError *err = NULL;
    GOptionEntry rentry[] = {
        {"run-id", 'r', 0, G_OPTION_ARG_INT, &runid, "P2SC runID", "N"},
        {NULL, 0, 0, G_OPTION_ARG_NONE, NULL, NULL, NULL}
    };
    GOptionContext *ctxt;

    ctxt = g_option_context_new(context);
    g_option_context_set_summary(ctxt, summary);
    g_option_context_add_main_entries(ctxt, rentry, NULL);
    if (entries)
        g_option_context_add_main_entries(ctxt, entries, NULL);

    ret = g_option_context_parse(ctxt, argc, argv, &err);

    base_prog = g_path_get_basename((*argv)[0]);
    if (*argc == 2)
        base_file = g_path_get_basename((*argv)[1]);

    p2sc_init(base_prog, appname, base_file, runid);

    if (!ret || err) {
        if (err && err->message) {
            P2SC_Msg(LVL_FATAL, "Option parsing failed: %s", err->message);
            g_error_free(err);
        } else
            P2SC_Msg(LVL_FATAL, "Option parsing failed: unknown reason");
    }
    if (marg > 0 && (*argc - 1) < marg) {
        char *help = g_option_context_get_help(ctxt, TRUE, NULL);
        printf("%s", help);
        g_free(help);

        P2SC_Msg(LVL_FATAL, "At least %d argument(s) mandatory", marg);
    }

    g_free(base_file);
    g_free(base_prog);
    g_option_context_free(ctxt);
}

void p2sc_option(int argc, char **argv, const char *appname,
                 const char *context, const char *summary,
                 const GOptionEntry * entries)
{
    p2sc_option_ext(1, &argc, &argv, appname, context, summary, entries);
}

int p2sc_spawn(const char *cmd, char **sout, char **serr)
{
    int stat;
    GError *err = NULL;

    if (!g_spawn_command_line_sync(cmd, sout, serr, &stat, &err)) {
        if (err && err->message) {
            P2SC_Msg(LVL_FATAL, "spawn(%s): %s", cmd, err->message);
            g_error_free(err);
        } else
            P2SC_Msg(LVL_FATAL, "spawn(%s): unknown failure", cmd);
    }

    if (stat)
        P2SC_Msg(LVL_WARNING,
                 "%s failed with exit status:%d\nstdout: %s\nstderr: %s",
                 cmd, stat, (*sout && **sout) ? *sout : "(null)",
                 (*serr && **serr) ? *serr : "(null)");

    return stat;
}

void p2sc_spawn_many(const char **cmd, int num)
{
    int i, status;
    GError *err = NULL;
    GPid *pid = (GPid *) g_malloc(num * sizeof *pid);

    for (i = 0; i < num; ++i) {
        gint argc;
        gchar **argv;

        if (!g_shell_parse_argv(cmd[i], &argc, &argv, &err)) {
            if (err && err->message) {
                P2SC_Msg(LVL_FATAL, "shell_parse_argv(%s): %s", cmd[i],
                         err->message);
                g_error_free(err);
            } else
                P2SC_Msg(LVL_FATAL, "shell_parse_argv(%s): unknown failure",
                         cmd[i]);
        }

        if (!g_spawn_async(NULL, argv, NULL,
                           (GSpawnFlags) (G_SPAWN_SEARCH_PATH |
                                          G_SPAWN_STDOUT_TO_DEV_NULL |
                                          G_SPAWN_STDERR_TO_DEV_NULL |
                                          G_SPAWN_DO_NOT_REAP_CHILD),
                           NULL, NULL, pid + i, &err)) {
            if (err && err->message) {
                P2SC_Msg(LVL_FATAL, "spawn_async(%s): %s", cmd[i],
                         err->message);
                g_error_free(err);
            } else
                P2SC_Msg(LVL_FATAL, "spawn_aync(%s): unknown failure", cmd[i]);
        }

        g_strfreev(argv);
    }
    /* wait for our children */
    for (i = 0; i < num; ++i) {
        waitpid(pid[i], &status, 0);
        g_spawn_close_pid(pid[i]);
    }

    g_free(pid);
}

int p2sc_spawn_lmat(const char *app, const char *args, char **sout, char **serr)
{
    char *cmd =
        g_strdup_printf("/p2sc/bin/LMAT/scheduler.pl --cron %s --args \"%s\"",
                        app, args);
    int ret = p2sc_spawn(cmd, sout, serr);

    g_free(cmd);
    return ret;
}

void *p2sc_get_symbol(const char *path, const char **name, void **addr)
{
    if (!g_module_supported())
        P2SC_Msg(LVL_FATAL_INTERNAL_ERROR,
                 "Dynamic loading of modules not supported");

    GModule *module = g_module_open(path,
                                    (GModuleFlags) (G_MODULE_BIND_LAZY |
                                                    G_MODULE_BIND_LOCAL));

    if (!module)
        P2SC_Msg(LVL_FATAL_INTERNAL_ERROR, "%s", g_module_error());

    while (*name)
        if (!g_module_symbol(module, *name++, addr++))
            P2SC_Msg(LVL_FATAL_INTERNAL_ERROR, "%s", g_module_error());

    return module;
}

int p2sc_string_isnumeric(const char *s)
{
    if (!s || !*s)
        return 0;

    char *end = NULL;
    double d __attribute__ ((unused)) = g_ascii_strtod(s, &end);
    if ((!errno || errno == ERANGE) && (!end || !*end))
        return 1;

    return 0;
}

static inline guint8 h2b(char c)
{
    switch (c) {
    case '0':
        return 0;
    case '1':
        return 1;
    case '2':
        return 2;
    case '3':
        return 3;
    case '4':
        return 4;
    case '5':
        return 5;
    case '6':
        return 6;
    case '7':
        return 7;
    case '8':
        return 8;
    case '9':
        return 9;
    case 'a':
    case 'A':
        return 10;
    case 'b':
    case 'B':
        return 11;
    case 'c':
    case 'C':
        return 12;
    case 'd':
    case 'D':
        return 13;
    case 'e':
    case 'E':
        return 14;
    case 'f':
    case 'F':
        return 15;
    }
    return 0xff;
}

char *p2sc_bin2hex(const guint8 * bin, size_t len)
{
    static const char hexdig[] = "0123456789ABCDEF";

    size_t alen = len * 2;
    char *hex = (char *) g_malloc(alen + 1);

    for (size_t i = 0; i < len; ++i) {
        guint8 b = bin[i];

        hex[2 * i] = hexdig[b >> 4];
        hex[2 * i + 1] = hexdig[b & 0xf];
    }
    hex[alen] = 0;

    return hex;
}

guint8 *p2sc_hex2bin(const char *hex, size_t len)
{
    len /= 2;
    guint8 *bin = (guint8 *) g_malloc(len);

    for (size_t i = 0; i < len; ++i)
        bin[i] = (h2b(hex[2 * i]) << 4) + (h2b(hex[2 * i + 1]) & 0xf);

    return bin;
}
