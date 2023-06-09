// VariantKey Fast Encoder Command Line Application
//
// vk.c
//
// @category   Tools
// @author     Nicola Asuni <info@tecnick.com>
// @link       https://github.com/tecnickcom/variantkey
// @license    MIT [LICENSE](https://raw.githubusercontent.com/tecnickcom/variantkey/main/LICENSE)

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/variantkey/variantkey.h"

#ifndef VERSION
#define VERSION "0.0.0-0"
#endif

int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        fprintf(stderr, "VariantKey Encoder %s\nUsage: vk CHROM POS REF ALT\n", VERSION);
        return 1;
    }
    fprintf(stdout, "%016" PRIx64, variantkey(argv[1], strlen(argv[1]), strtoull(argv[2], NULL, 10), argv[3], strlen(argv[3]), argv[4], strlen(argv[4])));
}
