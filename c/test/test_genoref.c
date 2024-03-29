// VariantKey
//
// test_genoref.c
//
// @category   Test
// @author     Nicola Asuni <info@tecnick.com>
// @link       https://github.com/tecnickcom/variantkey
// @license    MIT [LICENSE](https://raw.githubusercontent.com/tecnickcom/variantkey/main/LICENSE)

// Test for genoref

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <sys/mman.h>
#include "../src/variantkey/genoref.h"

// returns current time in nanoseconds
uint64_t get_time()
{
    struct timespec t;
    (void) timespec_get(&t, TIME_UTC);
    return (((uint64_t)t.tv_sec * 1000000000) + (uint64_t)t.tv_nsec);
}

int test_aztoupper()
{
    int errors = 0;
    int i = 0, c = 0;
    for (i=97 ; i <= 122; i++)
    {
        c = aztoupper(i);
        if (c != (i - 32))
        {
            (void) fprintf(stderr, "%s : Wrong uppercase value for %d - expecting %d, got %d\n", __func__, i, (i - 32), c);
            ++errors;
        }
    }
    c = aztoupper(96);
    if (c != 96)
    {
        (void) fprintf(stderr, "%s : Wrong uppercase value - expecting 96, got %d\n", __func__, c);
        ++errors;
    }
    return errors;
}

void benchmark_aztoupper()
{
    int c = 0;
    uint64_t tstart = 0, tend = 0;
    int i = 0, j = 0;
    int size = 100000;
    tstart = get_time();
    for (i=0 ; i < size; i++)
    {
        for (j=0 ; j < 256; j++)
        {
            c = aztoupper(j);
        }
    }
    tend = get_time();
    (void) fprintf(stdout, " * %s : %lu ns/op (%d)\n", __func__, (tend - tstart)/(uint64_t)(size*256), c);
}

int test_prepend_char()
{
    int errors = 0;
    char original[5] =   "BCD";
    char expected[5] = "ABCD";
    size_t size = 3;
    prepend_char('A', original, &size);
    if (size != 4)
    {
        (void) fprintf(stderr, "%s : Expected size 4, got %lu\n", __func__, size);
        ++errors;
    }
    if (strcmp(original, expected) != 0)
    {
        (void) fprintf(stderr, "%s : Expected %s, got %s\n", __func__, expected, original);
        ++errors;
    }
    return errors;
}

void benchmark_prepend_char()
{
    char original[1002] =   "B";
    size_t len = 1;
    uint64_t tstart = 0, tend = 0;
    int i = 0;
    int size = 1000;
    tstart = get_time();
    for (i=0 ; i < size; i++)
    {
        prepend_char('A', original, &len);
    }
    tend = get_time();
    (void) fprintf(stdout, " * %s : %lu ns/op\n", __func__, (tend - tstart)/size);
}

int test_swap_sizes()
{
    int errors = 0;
    size_t first = 123;
    size_t second = 456;
    swap_sizes(&first, &second);
    if ((first != 456) || (second != 123))
    {
        (void) fprintf(stderr, "%s : Error while swapping sizes\n", __func__);
        ++errors;
    }
    return errors;
}

int test_swap_alleles()
{
    int errors = 0;
    char first[ALLELE_MAXSIZE] = "ABC";
    char second[ALLELE_MAXSIZE] = "DEFGHI";
    size_t sizefirst = 3;
    size_t sizesecond = 6;
    swap_alleles(first, &sizefirst, second, &sizesecond);
    if ((strcmp(first, "DEFGHI") != 0) || (strcmp(second, "ABC") != 0) || (sizefirst != 6) || (sizesecond != 3))
    {
        (void) fprintf(stderr, "%s : Error while swapping alleles: %s %s\n", __func__, first, second);
        ++errors;
    }
    return errors;
}

int test_get_genoref_seq(mmfile_t mf)
{
    int errors = 0;
    char ref = 0, exp = 0;
    uint8_t chrom = 0;
    for (chrom = 1; chrom <= 25; chrom++)
    {
        ref = get_genoref_seq(mf, chrom, 0); // first
        if (ref != 'A')
        {
            (void) fprintf(stderr, "%s (%d) (first): Expected reference 'A', got '%c'\n", __func__, chrom, ref);
            ++errors;
        }
        ref = get_genoref_seq(mf, chrom, (26 - chrom)); // last
        exp = (char) ('Z' + 1 - chrom);
        if (ref != exp)
        {
            (void) fprintf(stderr, "%s (%d) (last): Expected reference '%c', got '%c'\n", __func__, chrom, exp, ref);
            ++errors;
        }
        ref = get_genoref_seq(mf, chrom, (27 - chrom)); // invalid
        if (ref != 0)
        {
            (void) fprintf(stderr, "%s (%d) (invalid): Expected reference 0, got '%c'\n", __func__, chrom, ref);
            ++errors;
        }
    }
    return errors;
}

void benchmark_get_genoref_seq(mmfile_t mf)
{
    uint8_t chrom = 0;
    uint64_t tstart = 0, tend = 0;
    int i = 0;
    int size = 100000;
    tstart = get_time();
    for (i=0 ; i < size; i++)
    {
        for (chrom = 1; chrom <= 25; chrom++)
        {
            get_genoref_seq(mf, chrom, 1);
        }
    }
    tend = get_time();
    (void) fprintf(stdout, " * %s : %lu ns/op\n", __func__, (tend - tstart)/(uint64_t)(size*25));
}

int test_check_reference(mmfile_t mf)
{
    int errors = 0;
    int ret = 0;
    int i = 0;
    typedef struct test_ref_t
    {
        int        exp;
        uint8_t    chrom;
        uint32_t   pos;
        size_t     sizeref;
        const char *ref;
    } test_ref_t;
    static test_ref_t test_ref[42] =
    {
        { 0, 1,   0,  1, "A"                         },
        { 0, 1,  25,  1, "Z"                         },
        { 0, 25,  0,  1, "A"                         },
        { 0, 25,  1,  1, "B"                         },
        { 0, 2,   0, 25, "ABCDEFGHIJKLmnopqrstuvwxy" },
        {-2, 1,  26,  4, "ZABC"                      },
        {-1, 1,   0, 26, "ABCDEFGHIJKLmnopqrstuvwxyJ"},
        {-1, 14,  2,  3, "ZZZ"                       },
        { 1, 1,   0,  1, "N"                         },
        { 1, 10, 13,  1, "A"                         },
        { 1, 1,   3,  1, "B"                         },
        { 1, 1,   1,  1, "C"                         },
        { 1, 1,   0,  1, "D"                         },
        { 1, 1,   3,  1, "A"                         },
        { 1, 1,   0,  1, "H"                         },
        { 1, 1,   7,  1, "A"                         },
        { 1, 1,   0,  1, "V"                         },
        { 1, 1,  21,  1, "A"                         },
        { 1, 1,   0,  1, "W"                         },
        { 1, 1,  19,  1, "W"                         },
        { 1, 1,  22,  1, "A"                         },
        { 1, 1,  22,  1, "T"                         },
        { 1, 1,   2,  1, "S"                         },
        { 1, 1,   6,  1, "S"                         },
        { 1, 1,  18,  1, "C"                         },
        { 1, 1,  18,  1, "G"                         },
        { 1, 1,   0,  1, "M"                         },
        { 1, 1,   2,  1, "M"                         },
        { 1, 1,  12,  1, "A"                         },
        { 1, 1,  12,  1, "C"                         },
        { 1, 1,   6,  1, "K"                         },
        { 1, 1,  19,  1, "K"                         },
        { 1, 1,  10,  1, "G"                         },
        { 1, 1,  10,  1, "T"                         },
        { 1, 1,   0,  1, "R"                         },
        { 1, 1,   6,  1, "R"                         },
        { 1, 1,  17,  1, "A"                         },
        { 1, 1,  17,  1, "G"                         },
        { 1, 1,   2,  1, "Y"                         },
        { 1, 1,  19,  1, "Y"                         },
        { 1, 1,  24,  1, "C"                         },
        { 1, 1,  24,  1, "T"                         },
    };
    for (i = 0; i < 42; i++)
    {
        ret = check_reference(mf, test_ref[i].chrom, test_ref[i].pos, test_ref[i].ref, test_ref[i].sizeref);
        if (ret != test_ref[i].exp)
        {
            (void) fprintf(stderr, "%s (%d): Expected %d, got %d\n", __func__, i, test_ref[i].exp, ret);
            ++errors;
        }
    }
    return errors;
}

int test_flip_allele()
{
    int errors = 0;
    char allele[] = "ATCGMKRYBVDHWSNatcgmkrybvdhwsn";
    const char expected[] = "TAGCKMYRVBHDWSNTAGCKMYRVBHDWSN";
    flip_allele(allele, 30);
    if (strcmp(allele, expected) != 0)
    {
        (void) fprintf(stderr, "%s : Expected %s, got %s\n", __func__, expected, allele);
        ++errors;
    }
    return errors;
}

void benchmark_flip_allele()
{
    char allele[] =   "ATCGMKRYBVDHWSNatcgmkrybvdhwsn";
    uint64_t tstart = 0, tend = 0;
    int i = 0;
    int size = 100000;
    tstart = get_time();
    for (i=0 ; i < size; i++)
    {
        flip_allele(allele, 30);
    }
    tend = get_time();
    (void) fprintf(stdout, " * %s : %lu ns/op\n", __func__, (tend - tstart)/(uint64_t)(size*25));
}

int test_normalize_variant(mmfile_t mf)
{
    int errors = 0;
    int ret = 0;
    int i = 0;
    typedef struct test_norm_t
    {
        int        exp;
        uint8_t    chrom;
        uint32_t   pos;
        uint32_t   exp_pos;
        size_t     sizeref;
        size_t     sizealt;
        size_t     exp_sizeref;
        size_t     exp_sizealt;
        const char *exp_ref;
        const char *exp_alt;
        char       ref[256];
        char       alt[256];
    } test_norm_t;
    static test_norm_t test_norm[12] =
    {
        {-2, 1, 26, 26, 1, 1, 1, 1, "A",  "C",  "A",      "C"     },  // invalid position
        {-1, 1,  0,  0, 1, 1, 1, 1, "J",  "C",  "J",      "C"     },  // invalid reference
        { 4, 1,  0,  0, 1, 1, 1, 1, "A",  "C",  "T",      "G"     },  // flip
        { 0, 1,  0,  0, 1, 1, 1, 1, "A",  "C",  "A",      "C"     },  // OK
        {32, 13, 2,  3, 3, 2, 2, 1, "DE", "D",  "CDE",    "CD"    },  // left trim
        {48, 13, 2,  3, 3, 3, 1, 1, "D",  "F",  "CDE",    "CFE"   },  // left trim + right trim
        {48, 1,  0,  2, 6, 6, 1, 1, "C",  "K",  "aBCDEF", "aBKDEF"},  // left trim + right trim
        { 0, 1,  0,  0, 1, 0, 1, 0, "A",  "",   "A",      ""      },  // OK
        { 8, 1,  3,  2, 1, 0, 2, 1, "CD", "C",  "D",      ""      },  // left extend
        { 0, 1, 24, 24, 1, 2, 1, 2, "Y",  "CK", "Y",      "CK"    },  // OK
        { 2, 1,  0,  0, 1, 1, 1, 1, "A",  "G",  "G",      "A"     },  // swap
        { 6, 1,  0,  0, 1, 1, 1, 1, "A",  "C",  "G",      "T"     },  // swap + flip
    };
    for (i = 0; i < 12; i++)
    {
        ret = normalize_variant(mf, test_norm[i].chrom, &test_norm[i].pos, test_norm[i].ref, &test_norm[i].sizeref, test_norm[i].alt, &test_norm[i].sizealt);
        if (ret != test_norm[i].exp)
        {
            (void) fprintf(stderr, "%s (%d): Expected return value %d, got %d\n", __func__, i, test_norm[i].exp, ret);
            ++errors;
        }
        if (test_norm[i].pos != test_norm[i].exp_pos)
        {
            (void) fprintf(stderr, "%s (%d): Expected POS %" PRIu32 ", got %" PRIu32 "\n", __func__, i, test_norm[i].exp_pos, test_norm[i].pos);
            ++errors;
        }
        if (test_norm[i].sizeref != test_norm[i].exp_sizeref)
        {
            (void) fprintf(stderr, "%s (%d): Expected REF size %lu, got %lu\n", __func__, i, test_norm[i].exp_sizeref, test_norm[i].sizeref);
            ++errors;
        }
        if (test_norm[i].sizealt != test_norm[i].exp_sizealt)
        {
            (void) fprintf(stderr, "%s (%d): Expected ALT size %lu, got %lu\n", __func__, i, test_norm[i].exp_sizealt, test_norm[i].sizealt);
            ++errors;
        }
        if (strcmp(test_norm[i].ref, test_norm[i].exp_ref) != 0)
        {
            (void) fprintf(stderr, "%s (%d): Expected REF %s, got %s\n", __func__, i, test_norm[i].exp_ref, test_norm[i].ref);
            ++errors;
        }
        if (strcmp(test_norm[i].alt, test_norm[i].exp_alt) != 0)
        {
            (void) fprintf(stderr, "%s (%d): Expected ALT %s, got %s\n", __func__, i, test_norm[i].exp_alt, test_norm[i].alt);
            ++errors;
        }
    }
    return errors;
}

int test_normalized_variantkey(mmfile_t mf)
{
    int errors = 0;
    int ret = 0;
    int i = 0;
    uint64_t vk = 0;
    typedef struct test_nvk_t
    {
        int        exp;
        char       chrom[3];
        uint8_t    posindex;
        uint32_t   pos;
        uint32_t   exp_pos;
        size_t     sizeref;
        size_t     sizealt;
        size_t     exp_sizeref;
        size_t     exp_sizealt;
        uint64_t   vk;
        const char *exp_ref;
        const char *exp_alt;
        char       ref[256];
        char       alt[256];
    } test_nvk_t;
    static test_nvk_t test_nvk[12] =
    {
        {-2, "1",  0, 26, 26, 1, 1, 1, 1, 0x0800000d08880000, "A",  "C",  "A",      "C"     },  // invalid position
        {-1, "1",  1,  1,  0, 1, 1, 1, 1, 0x08000000736a947f, "J",  "C",  "J",      "C"     },  // invalid reference
        { 4, "1",  0,  0,  0, 1, 1, 1, 1, 0x0800000008880000, "A",  "C",  "T",      "G"     },  // flip
        { 0, "1",  0,  0,  0, 1, 1, 1, 1, 0x0800000008880000, "A",  "C",  "A",      "C"     },  // OK
        {32, "13", 1,  3,  3, 3, 2, 2, 1, 0x68000001fed6a22d, "DE", "D",  "CDE",    "CD"    },  // left trim
        {48, "13", 0,  2,  3, 3, 3, 1, 1, 0x68000001c7868961, "D",  "F",  "CDE",    "CFE"   },  // left trim + right trim
        {48, "1",  0,  0,  2, 6, 6, 1, 1, 0x0800000147df7d13, "C",  "K",  "aBCDEF", "aBKDEF"},  // left trim + right trim
        { 0, "1",  0,  0,  0, 1, 0, 1, 0, 0x0800000008000000, "A",  "",   "A",      ""      },  // OK
        { 8, "1",  0,  3,  2, 1, 0, 2, 1, 0x0800000150b13d0f, "CD", "C",  "D",      ""      },  // left extend
        { 0, "1",  1, 25, 24, 1, 2, 1, 2, 0x0800000c111ea6eb, "Y",  "CK", "Y",      "CK"    },  // OK
        { 2, "1",  0,  0,  0, 1, 1, 1, 1, 0x0800000008900000, "A",  "G",  "G",      "A"     },  // swap
        { 6, "1",  1,  1,  0, 1, 1, 1, 1, 0x0800000008880000, "A",  "C",  "G",      "T"     },  // swap + flip
    };
    for (i = 0; i < 12; i++)
    {
        ret = 0;
        vk = normalized_variantkey(mf, test_nvk[i].chrom, strlen(test_nvk[i].chrom), &test_nvk[i].pos, test_nvk[i].posindex, test_nvk[i].ref, &test_nvk[i].sizeref, test_nvk[i].alt, &test_nvk[i].sizealt, &ret);
        if (vk != test_nvk[i].vk)
        {
            (void) fprintf(stderr, "%s (%d): Expected return value %016" PRIx64 " , got  %016" PRIx64 "\n", __func__, i, test_nvk[i].vk, vk);
            ++errors;
        }
        if (ret != test_nvk[i].exp)
        {
            (void) fprintf(stderr, "%s (%d): Expected return value %d, got %d\n", __func__, i, test_nvk[i].exp, ret);
            ++errors;
        }
        if (test_nvk[i].pos != test_nvk[i].exp_pos)
        {
            (void) fprintf(stderr, "%s (%d): Expected POS %" PRIu32 ", got %" PRIu32 "\n", __func__, i, test_nvk[i].exp_pos, test_nvk[i].pos);
            ++errors;
        }
        if (test_nvk[i].sizeref != test_nvk[i].exp_sizeref)
        {
            (void) fprintf(stderr, "%s (%d): Expected REF size %lu, got %lu\n", __func__, i, test_nvk[i].exp_sizeref, test_nvk[i].sizeref);
            ++errors;
        }
        if (test_nvk[i].sizealt != test_nvk[i].exp_sizealt)
        {
            (void) fprintf(stderr, "%s (%d): Expected ALT size %lu, got %lu\n", __func__, i, test_nvk[i].exp_sizealt, test_nvk[i].sizealt);
            ++errors;
        }
        if (strcmp(test_nvk[i].ref, test_nvk[i].exp_ref) != 0)
        {
            (void) fprintf(stderr, "%s (%d): Expected REF %s, got %s\n", __func__, i, test_nvk[i].exp_ref, test_nvk[i].ref);
            ++errors;
        }
        if (strcmp(test_nvk[i].alt, test_nvk[i].exp_alt) != 0)
        {
            (void) fprintf(stderr, "%s (%d): Expected ALT %s, got %s\n", __func__, i, test_nvk[i].exp_alt, test_nvk[i].alt);
            ++errors;
        }
    }
    return errors;
}

int main()
{
    int errors = 0;
    int err = 0;

    mmfile_t genoref = {0};
    mmap_genoref_file("genoref.bin", &genoref);

    errors += test_aztoupper();
    errors += test_prepend_char();
    errors += test_swap_sizes();
    errors += test_swap_alleles();
    errors += test_get_genoref_seq(genoref);
    errors += test_check_reference(genoref);
    errors += test_flip_allele();
    errors += test_normalize_variant(genoref);
    errors += test_normalized_variantkey(genoref);

    benchmark_aztoupper();
    benchmark_prepend_char();
    benchmark_get_genoref_seq(genoref);
    benchmark_flip_allele();

    err = munmap_binfile(genoref);
    if (err != 0)
    {
        (void) fprintf(stderr, "Got %d error while unmapping the genoref file\n", err);
        return 1;
    }

    return errors;
}
