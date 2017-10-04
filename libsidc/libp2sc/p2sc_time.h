/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

#ifndef __P2SC_TIME_H__
#define __P2SC_TIME_H__

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------- */

#define OBET_RES   0x1p-14
#define OBET_MAX   (((guint64) 1 << 46) - 1)
#define SIZEOF_CUC 7

    guint64 p2sc_cuc2ticks(const guint8 t[SIZEOF_CUC]);
    int p2sc_ascii2cuc(guint8 t[SIZEOF_CUC], const char *in);

    int p2sc_string2date(const char *, double d[6]);
    char *p2sc_date2string(const double d[6]);

    char *p2sc_timestamp(double, int);

#define p2sc_ticks2double(ticks) (((guint64) (ticks) & 0x3fffffffffffULL) * OBET_RES)
#define p2sc_double2ticks(secs)  ((guint64) ((double) (secs) * (1. / OBET_RES) + .5))

#ifndef NO_TRACE_TIME

#include <sys/times.h>
#include <unistd.h>
#include <stdio.h>

#define START_TIME \
{ \
    struct tms _tbeg, _tend; \
    double _tick_tock = sysconf(_SC_CLK_TCK); \
    clock_t _rend, _rbeg = times(&_tbeg); \

#define STOP_TIME \
    _rend = times(&_tend); \
    fprintf(stderr, \
        "\n\treal:   %.2fs" \
        "\n\tuser:   %.2fs" \
        "\n\tsystem: %.2fs\n", \
        (_rend - _rbeg) / _tick_tock, \
        (_tend.tms_utime - _tbeg.tms_utime) / _tick_tock, \
        (_tend.tms_stime - _tbeg.tms_stime) / _tick_tock); \
}

    typedef unsigned long long _cputicks;

    /* *INDENT-OFF* */
    static __inline__ _cputicks rdtsc_start(void) {
        unsigned cycles_low, cycles_high;
        __asm__ volatile ("CPUID\n\t"
                          "RDTSC\n\t"
                          "mov %%edx, %0\n\t"
                          "mov %%eax, %1\n\t":"=r" (cycles_high),
                          "=r"(cycles_low)::"%rax", "%rbx", "%rcx", "%rdx");
         return ((_cputicks) cycles_high << 32) | cycles_low;
    }

    static __inline__ _cputicks rdtsc_stop(void) {
        unsigned cycles_low, cycles_high;
        __asm__ volatile ("RDTSCP\n\t"
                          "mov %%edx, %0\n\t"
                          "mov %%eax, %1\n\t"
                          "CPUID\n\t":"=r" (cycles_high),
                          "=r"(cycles_low)::"%rax", "%rbx", "%rcx", "%rdx");
        return ((_cputicks) cycles_high << 32) | cycles_low;
    }
    /* *INDENT-ON* */

#else

#define START_TIME
#define STOP_TIME

#endif

/* ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif
#endif
