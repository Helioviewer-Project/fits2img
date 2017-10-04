
/* Author: Bogdan Nicula, ROB */

#include <stdio.h>
#include <string.h>
#include <glib.h>

#ifdef strlcat
#undef strlcat
#endif

#ifdef strlcpy
#undef strlcpy
#endif

#include "idl_export.h"

#include "swap_math.h"
#include "swap_warp.h"

int IDL_Load(void);

static void flip(const float *in, size_t w, size_t h, float *out)
{
    for (size_t j = 0; j < h; ++j)
        memcpy(out + j * w, in + (h - 1 - j) * w, w * sizeof *in);
}

static IDL_VPTR sa(int argc __attribute__ ((unused)), IDL_VPTR argv[],
                   char *argk __attribute__ ((unused)))
{
    IDL_VPTR ain = IDL_BasicTypeConversion(1, &argv[0], IDL_TYP_FLOAT);
    IDL_ARRAY *arr = ain->value.arr;

    size_t w = arr->dim[0], h = arr->dim[1];
    float *in = (float *) g_malloc(w * h * sizeof *in);

    flip((const float *) arr->data, w, h, in);

    IDL_ULONG ow = IDL_ULongScalar(argv[7]), oh = IDL_ULongScalar(argv[8]);

    swap_bicubic_t *f = swap_bicubic_alloc(0, 0.5);
    float *a = swap_affine(f, in, w, h,
                           IDL_DoubleScalar(argv[1]), IDL_DoubleScalar(argv[2]),
                           IDL_DoubleScalar(argv[3]), IDL_DoubleScalar(argv[4]),
                           IDL_DoubleScalar(argv[5]), IDL_DoubleScalar(argv[6]),
                           ow, oh);
    swap_bicubic_free(f);
    g_free(in);

    IDL_VPTR ret;
    IDL_MEMINT dim[] = { ow, oh };

    flip(a, ow, oh,
         (float *) IDL_MakeTempArray(IDL_TYP_FLOAT, 2, dim, IDL_ARR_INI_NOP,
                                     &ret));
    g_free(a);

    return ret;
}

int IDL_Load(void)
{
    static IDL_SYSFUN_DEF2 fun[] = {
        {(IDL_SYSRTN_FUN) sa, "SWAP_AFFINE", 9, 9, 0, NULL}
    };
    return IDL_SysRtnAdd(fun, IDL_TRUE, IDL_CARRAY_ELTS(fun));
}
