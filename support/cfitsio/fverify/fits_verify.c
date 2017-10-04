
#include <stdio.h>
#include "fverify.h"

int main(int argc, char **argv)
{
    if (argc < 2)
        return 1;

    verify_fits(argv[1], stdout);

    return 0;
}
