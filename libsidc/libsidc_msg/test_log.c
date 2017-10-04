
#include <stdio.h>
#include <glib.h>

#include "sidc_log.h"

#include <sys/times.h>
#include <unistd.h>

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

int main(void)
{

	START_TIME;

	for (int i = 0; i < 1000000; ++i) {
		GVariant *v = sidc_log("SWTMR",
							   "$Id: swap_reformat.c 5012 2013-12-19 09:52:49Z bogdan $",
							   107044,
							   "Uncompressed image data shorter than expected:",
							   4000,
							   "swap_reformat - data_unpack() in swap_reformat.c:27",
							   NULL);

		sidc_unlog(g_variant_get_data(v), g_variant_get_size(v));

		g_variant_unref(v);
	}

	STOP_TIME;

	return 0;
}
