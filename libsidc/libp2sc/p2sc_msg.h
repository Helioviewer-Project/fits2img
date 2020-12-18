/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

#ifndef __P2SC_MSG_H__
#define __P2SC_MSG_H__

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------- */

#include <stdlib.h>

#define LVL_FATAL   4000
#define LVL_WARNING 3000
#define LVL_MESSAGE 2000
#define LVL_CONTROL 1000

#define _ARGUMENTS           100
#define _FILESYSTEM          200
#define _CORRUPT_INPUT_DATA  300
#define _COMMANDING          400
#define _FITS                600
#define _CORRUPT_OUTPUT_DATA 700
#define _INTERNAL_ERROR      800
#define _PPT                 999

#define LVL_WARNING_ARGUMENTS          (LVL_WARNING + _ARGUMENTS)
#define LVL_WARNING_FILESYSTEM         (LVL_WARNING + _FILESYSTEM)
#define LVL_WARNING_CORRUPT_INPUT_DATA (LVL_WARNING + _CORRUPT_INPUT_DATA)
#define LVL_WARNING_COMMANDING         (LVL_WARNING + _COMMANDING)

#define LVL_FATAL_ARGUMENTS            (LVL_FATAL + _ARGUMENTS)
#define LVL_FATAL_FILESYSTEM           (LVL_FATAL + _FILESYSTEM)
#define LVL_FATAL_CORRUPT_INPUT_DATA   (LVL_FATAL + _CORRUPT_INPUT_DATA)
#define LVL_FATAL_FITS                 (LVL_FATAL + _FITS)
#define LVL_FATAL_CORRUPT_OUTPUT_DATA  (LVL_FATAL + _CORRUPT_OUTPUT_DATA)
#define LVL_FATAL_INTERNAL_ERROR       (LVL_FATAL + _INTERNAL_ERROR)
#define LVL_FATAL_PPT                  (LVL_FATAL + _PPT)

#define LVL_CONTROL_START              (LVL_CONTROL + 1)
#define LVL_CONTROL_FINISH_CONT        (LVL_CONTROL + 200)
#define LVL_CONTROL_FINISH_STOP        (LVL_CONTROL + 300)
#define LVL_CONTROL_FINISH_STOP_PPT    (LVL_CONTROL + 399)

/* use as P2SC_Msg(msgid, format, ...) - printf style */
/* ##__VA_ARGS__ is a GNU CC extension for limited C99 CPP behavior */
#define P2SC_Msg(_msgid_, ...) \
    do { \
        _p2sc_msg(__func__, __FILE__, __LINE__, _versionid_, _msgid_, __VA_ARGS__); \
        if (_msgid_ >= LVL_FATAL) \
            exit(1); \
    } while (0)

    void _p2sc_msg(const char *, const char *, int, const char *, int,
                   const char *, ...) __attribute__((format(printf, 6, 7)));

/* ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif
#endif
