// VariantKey Fast Encoder Command Line Application
//
// vk.c
//
// @category   Tools
// @author     Nicola Asuni <info@tecnick.com>
// @link       https://github.com/tecnickcom/variantkey
// @license    MIT [LICENSE](https://raw.githubusercontent.com/tecnickcom/variantkey/main/LICENSE)

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/variantkey/variantkey.h"

#ifndef VERSION
#define VERSION "0.0.0-0"
#endif

//!< Highest position representable in the 28 bit POS field
enum { VK_MAX_POS = 0x0FFFFFFF };

// Unlike the library, which trusts its input by design, this command line tool
// validates the POS argument before encoding.
int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        (void) fprintf(stderr, "VariantKey Encoder %s\nUsage: vk CHROM POS REF ALT\n", VERSION);
        return 1;
    }
    char *end = NULL;
    errno = 0;
    unsigned long long pos = strtoull(argv[2], &end, 10);
    if ((errno != 0) || (end == argv[2]) || (*end != '\0') || (argv[2][0] == '-') || (pos > VK_MAX_POS))
    {
        (void) fprintf(stderr, "vk: POS must be an integer between 0 and %d, got '%s'\n", VK_MAX_POS, argv[2]);
        return 1;
    }
    (void) fprintf(stdout, "%016" PRIx64, variantkey(argv[1], strlen(argv[1]), (uint32_t)pos, argv[3], strlen(argv[3]), argv[4], strlen(argv[4])));
    return 0;
}
