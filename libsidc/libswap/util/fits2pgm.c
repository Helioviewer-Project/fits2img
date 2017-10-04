
/* Author: Bogdan Nicula, ROB */

static const char _versionid_[] __attribute__ ((unused)) =
    "$Id: fits2pgm.c 5110 2014-06-19 12:37:15Z bogdan $";

#include <glib.h>

#include "p2sc_fits.h"
#include "swap_color.h"
#include "swap_file.h"

int main(int argc, char **argv)
{
    if (argc < 3)
        return 1;

    size_t w, h, i;
    sfts_t *f = sfts_openro(argv[1]);

    sfts_find_hdukey(f, "DATE-OBS");

    float *im = (float *) sfts_read_image(f, &w, &h, SFLOAT), v;
    g_free(sfts_free(f));

    guint16 *imout = (guint16 *) g_malloc(w * h * sizeof *imout);

    for (i = 0; i < w * h; ++i) {
        v = im[i];
        if (v < 0)
            v = 0;
        else if (v > 0xffff)
            v = 0xffff;

        imout[i] = v + .5;
    }

    swap_write_pgm(argv[2], imout, w, h, 0xffff);

    g_free(imout);
    g_free(im);

    return 0;
}
