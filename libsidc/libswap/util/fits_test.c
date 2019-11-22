
/* Author: Bogdan Nicula, ROB */

static const char _versionid_[] __attribute__ ((unused)) = "$Id: fits_test.c 5110 2014-06-19 12:37:15Z bogdan $";

#include <glib.h>

#include "p2sc_fits.h"
#include "p2sc_stdlib.h"

#define APP_NAME "fits_test"

int main(int argc, char **argv)
{
    GOptionEntry entries[] = {
        {NULL, 0, 0, G_OPTION_ARG_NONE, NULL, NULL, NULL}
    };

    p2sc_option(argc, argv, APP_NAME, "FILE - SWAP FITS test", "This program tests SWAP FITS files", entries);

    g_free(sfts_free(sfts_openro(argv[1])));

    return 0;
}
