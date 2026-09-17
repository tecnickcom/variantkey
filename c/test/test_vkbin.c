// VariantKey
//
// test_vkbin.c
//
// @category   Test
// @author     Nicola Asuni <info@tecnick.com>
// @link       https://github.com/tecnickcom/variantkey
// @license    MIT [LICENSE](https://raw.githubusercontent.com/tecnickcom/variantkey/main/LICENSE)

/*
 * End-to-end test of the vkbin command line tool.
 *
 * The tool is run as a subprocess over a committed TSV fixture and the
 * resulting BINSRC1 file is read back through the library, which checks the
 * header layout, the VariantKey encoding and the fixed-point scaling in one
 * pass. The failure cases assert that the tool rejects bad input instead of
 * writing a file that would be silently wrong at query time.
 *
 * VKBIN_PATH and VKBIN_DATA are supplied by the build.
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/variantkey/binsearch.h"
#include "../src/variantkey/variantkey.h"

#ifndef VKBIN_PATH
#define VKBIN_PATH "vkbin"
#endif

#ifndef VKBIN_DATA
#define VKBIN_DATA "vkbin.tsv"
#endif

#define VKBIN_OUT "test_vkbin.out.bin"

//!< Longest output path the tool accepts.
enum { VKBIN_PATH_LIMIT = 4096 };

//!< Most value columns the tool accepts.
enum { VKBIN_VALCOLS_LIMIT = 255 };

typedef struct vkbin_row_t
{
    const char *chrom;
    uint32_t    pos;    //!< 1-based, as in the input file
    const char *ref;
    const char *alt;
    uint32_t    rawq;   //!< Expected stored raw_score
    uint32_t    phredq; //!< Expected stored PHRED
} vkbin_row_t;

/*
 * Expected content of data/vkbin.tsv under "4:5:2.0 4:5:0".
 *
 * The rows exercise: the first encodable position, the three alternate alleles
 * at one position, a chromosome-length position, the observed score extremes,
 * both chromosome spellings, X, Y and MT, a value that rounds half away from
 * zero, and both ends of the raw_score encoding range.
 */
static const vkbin_row_t expected[] =
{
    {"chr1",  1,         "A", "C", 200000, 0},
    {"chr1",  10001,     "T", "A", 196132, 106466},
    {"chr1",  10001,     "T", "C", 196800, 131140}, // "-0.032" is -0.03200
    {"chr1",  10001,     "T", "G", 196280, 111839},
    {"chr1",  10002,     "A", "C", 196417, 116835},
    {"chr1",  248956422, "G", "A", 73100,  1},
    {"chr11", 5225450,   "A", "T", 655700, 8299450},
    {"22",    50818468,  "C", "T", 323457, 1250000}, // 1.234565 rounds up
    {"X",     156030895, "G", "C", 0,      9999999}, // raw_score at the floor
    {"Y",     57227415,  "A", "G", 510000, 4242000},
    {"MT",    16569,     "T", "C", 200001, 700000},
};

#define EXPECTED_ROWS (sizeof(expected) / sizeof(expected[0]))

/**
 * Run the tool with the given arguments and return its exit status.
 */
static int run_vkbin(const char *args, const char *input)
{
    char cmd[16384];
    (void) snprintf(cmd, sizeof(cmd), "%s %s < %s > /dev/null 2> /dev/null",
                    VKBIN_PATH, args, input);
    // Driving the tool as a subprocess is the point of this test, so the shell
    // it needs is not a finding here.
    int rv = system(cmd); // NOLINT(cert-env33-c)
    return (rv == -1) ? -1 : (rv / 256);
}

/**
 * Convert the fixture and check that the tool reports success.
 */
static int test_vkbin_convert(void)
{
    int status = run_vkbin("-o " VKBIN_OUT " 4:5:2.0 4:5:0", VKBIN_DATA);
    if (status != 0)
    {
        (void) fprintf(stderr, "%s: expected exit 0, got %d\n", __func__, status);
        return 1;
    }
    return 0;
}

/**
 * Check the BINSRC1 header that the tool wrote.
 */
static int test_vkbin_header(mmfile_t mf)
{
    int errors = 0;
    if (mf.ncols != 3)
    {
        (void) fprintf(stderr, "%s: expected 3 columns, got %u\n", __func__, mf.ncols);
        ++errors;
    }
    if (mf.nrows != EXPECTED_ROWS)
    {
        (void) fprintf(stderr, "%s: expected %zu rows, got %" PRIu64 "\n",
                       __func__, (size_t)EXPECTED_ROWS, mf.nrows);
        ++errors;
    }
    if ((mf.ctbytes[0] != 8) || (mf.ctbytes[1] != 4) || (mf.ctbytes[2] != 4))
    {
        (void) fprintf(stderr, "%s: expected item sizes 8,4,4, got %u,%u,%u\n",
                       __func__, mf.ctbytes[0], mf.ctbytes[1], mf.ctbytes[2]);
        ++errors;
    }
    // Every column must start on a multiple of its item size or binsearch
    // would read misaligned values.
    uint8_t i = 0;
    for (i = 0; i < mf.ncols; i++)
    {
        if ((mf.index[i] % mf.ctbytes[i]) != 0)
        {
            (void) fprintf(stderr, "%s: column %u offset %" PRIu64 " is misaligned\n",
                           __func__, i, mf.index[i]);
            ++errors;
        }
    }
    return errors;
}

/**
 * Look up every expected row and compare the stored values.
 */
static int test_vkbin_values(mmfile_t mf)
{
    int errors = 0;
    const uint64_t *keys = get_src_offset_uint64_t(mf.src, mf.index[0]);
    const uint32_t *raws = get_src_offset_uint32_t(mf.src, mf.index[1]);
    const uint32_t *phreds = get_src_offset_uint32_t(mf.src, mf.index[2]);
    size_t i = 0;
    for (i = 0; i < EXPECTED_ROWS; i++)
    {
        const vkbin_row_t *e = &expected[i];
        // The tool converts the 1-based input position to the 0-based one that
        // a VariantKey carries.
        uint64_t vk = variantkey(e->chrom, strlen(e->chrom), (e->pos - 1),
                                 e->ref, strlen(e->ref), e->alt, strlen(e->alt));
        uint64_t first = 0;
        uint64_t last = mf.nrows;
        uint64_t found = col_find_first_le_uint64_t(keys, &first, &last, vk);
        if (found >= mf.nrows)
        {
            (void) fprintf(stderr, "%s (%zu): %s:%" PRIu32 " %s>%s not found\n",
                           __func__, i, e->chrom, e->pos, e->ref, e->alt);
            ++errors;
            continue;
        }
        if (found != i)
        {
            (void) fprintf(stderr, "%s (%zu): expected row %zu, got %" PRIu64 "\n",
                           __func__, i, i, found);
            ++errors;
        }
        if (raws[found] != e->rawq)
        {
            (void) fprintf(stderr, "%s (%zu): expected raw %" PRIu32 ", got %" PRIu32 "\n",
                           __func__, i, e->rawq, raws[found]);
            ++errors;
        }
        if (phreds[found] != e->phredq)
        {
            (void) fprintf(stderr, "%s (%zu): expected phred %" PRIu32 ", got %" PRIu32 "\n",
                           __func__, i, e->phredq, phreds[found]);
            ++errors;
        }
    }
    return errors;
}

/**
 * Check that the stored keys are in ascending order, which binsearch requires.
 */
static int test_vkbin_sorted(mmfile_t mf)
{
    const uint64_t *keys = get_src_offset_uint64_t(mf.src, mf.index[0]);
    uint64_t i = 0;
    for (i = 1; i < mf.nrows; i++)
    {
        if (keys[i] < keys[i - 1])
        {
            (void) fprintf(stderr, "%s: row %" PRIu64 " breaks the sort order\n", __func__, i);
            return 1;
        }
    }
    return 0;
}

/**
 * Check that a variant which is not in the file is not reported as found.
 */
static int test_vkbin_notfound(mmfile_t mf)
{
    const uint64_t *keys = get_src_offset_uint64_t(mf.src, mf.index[0]);
    uint64_t vk = variantkey("chr1", 4, 99999, "A", 1, "T", 1);
    uint64_t first = 0;
    uint64_t last = mf.nrows;
    uint64_t found = col_find_first_le_uint64_t(keys, &first, &last, vk);
    if (found < mf.nrows)
    {
        (void) fprintf(stderr, "%s: an absent variant was reported at row %" PRIu64 "\n",
                       __func__, found);
        return 1;
    }
    return 0;
}

/**
 * Check that value columns beyond the given specifications are ignored.
 *
 * Taking a prefix of the value columns is what lets one invocation survive a
 * source file that gains columns later.
 */
static int test_vkbin_subset(void)
{
    int errors = 0;
    if (run_vkbin("-o " VKBIN_OUT " 4:5:2.0", VKBIN_DATA) != 0)
    {
        (void) fprintf(stderr, "%s: a single column specification was rejected\n", __func__);
        return 1;
    }
    mmfile_t mf = {0};
    mmap_binfile(VKBIN_OUT, &mf);
    if (mf.src == (uint8_t *)MAP_FAILED)
    {
        (void) fprintf(stderr, "%s: cannot map the output\n", __func__);
        return 1;
    }
    if ((mf.ncols != 2) || (mf.nrows != EXPECTED_ROWS))
    {
        (void) fprintf(stderr, "%s: expected 2 columns and %zu rows, got %u and %" PRIu64 "\n",
                       __func__, (size_t)EXPECTED_ROWS, mf.ncols, mf.nrows);
        ++errors;
    }
    else
    {
        const uint32_t *raws = get_src_offset_uint32_t(mf.src, mf.index[1]);
        if (raws[0] != expected[0].rawq)
        {
            (void) fprintf(stderr, "%s: expected raw %" PRIu32 ", got %" PRIu32 "\n",
                           __func__, expected[0].rawq, raws[0]);
            ++errors;
        }
    }
    (void) munmap_binfile(&mf);
    (void) remove(VKBIN_OUT);
    return errors;
}

/**
 * Check that the command line is validated.
 */
static int test_vkbin_args(void)
{
    int errors = 0;
    size_t i = 0;
    static const struct
    {
        const char *args;
        const char *desc;
    } cases[] =
    {
        {"-x 4:5",                                  "unknown option"},
        {"-o",                                      "output without its argument"},
        {"-o " VKBIN_OUT " -s",                     "skip count without its argument"},
        {"-o " VKBIN_OUT " -s x 4:5",               "skip count that is not a number"},
        {"-o " VKBIN_OUT " -s -1 4:5",              "negative skip count"},
        {"-o " VKBIN_OUT " -s 1x 4:5",              "skip count with trailing garbage"},
        {"-o " VKBIN_OUT " -s 99999999999999999999 4:5", "skip count out of range"},
        {"4:5",                                     "no output file"},
        {"-o " VKBIN_OUT " -",                      "a lone dash as a column specification"},
        {"-o no_such_dir/out.bin 4:5",              "an output in a missing directory"},
    };
    for (i = 0; i < (sizeof(cases) / sizeof(cases[0])); i++)
    {
        if (run_vkbin(cases[i].args, VKBIN_DATA) == 0)
        {
            (void) fprintf(stderr, "%s (%zu): expected a failure for %s\n",
                           __func__, i, cases[i].desc);
            ++errors;
        }
    }
    if (run_vkbin("-h", VKBIN_DATA) != 0)
    {
        (void) fprintf(stderr, "%s: the help was not printed\n", __func__);
        ++errors;
    }
    // An output path that leaves no room for the temporary name, and more
    // column specifications than the header can describe.
    static char args[8192];
    int n = snprintf(args, sizeof(args), "-o ");
    memset((args + n), 'x', VKBIN_PATH_LIMIT);
    (void) snprintf((args + n + VKBIN_PATH_LIMIT), (sizeof(args) - (size_t)n - VKBIN_PATH_LIMIT), " 4:5");
    if (run_vkbin(args, VKBIN_DATA) == 0)
    {
        (void) fprintf(stderr, "%s: an over-long output path was accepted\n", __func__);
        ++errors;
    }
    n = snprintf(args, sizeof(args), "-o " VKBIN_OUT);
    for (i = 0; i <= VKBIN_VALCOLS_LIMIT; i++)
    {
        n += snprintf((args + n), (sizeof(args) - (size_t)n), " 1:0");
    }
    if (run_vkbin(args, VKBIN_DATA) == 0)
    {
        (void) fprintf(stderr, "%s: too many column specifications were accepted\n", __func__);
        ++errors;
    }
    return errors;
}

/**
 * Check that the tool rejects input it cannot encode correctly.
 */
static int test_vkbin_errors(void)
{
    int errors = 0;
    size_t i = 0;
    static const struct
    {
        const char *args;
        const char *desc;
    } cases[] =
    {
        {"-o " VKBIN_OUT " 3:5:2.0 4:5:0",   "width that is not 1, 2, 4 or 8"},
        {"-o " VKBIN_OUT " 4:5:2.0 4",       "column spec without decimals"},
        {"-o " VKBIN_OUT " 1:5:2.0 4:5:0",   "value too wide for the column"},
        {"-o " VKBIN_OUT " 4:5 4:5:0",       "negative value without an offset"},
        {"-o " VKBIN_OUT " -s 0 4:5:2.0 4:5:0", "header line parsed as data"},
        {"-o " VKBIN_OUT,                    "no column specification"},
    };
    for (i = 0; i < (sizeof(cases) / sizeof(cases[0])); i++)
    {
        int status = run_vkbin(cases[i].args, VKBIN_DATA);
        if (status == 0)
        {
            (void) fprintf(stderr, "%s (%zu): expected a failure for %s\n",
                           __func__, i, cases[i].desc);
            ++errors;
        }
    }
    // An unsorted input must be refused: the file would map without error and
    // return wrong answers at query time.
    FILE *f = fopen("test_vkbin_unsorted.tsv", "wb");
    if (f == NULL)
    {
        (void) fprintf(stderr, "%s: cannot create the unsorted fixture\n", __func__);
        return (errors + 1);
    }
    (void) fprintf(f, "#CHROM\tPOS\tREF\tALT\tV\n" "chr2\t100\tA\tC\t1.0\n" "chr1\t100\tA\tC\t2.0\n");
    (void) fclose(f);
    if (run_vkbin("-o " VKBIN_OUT " 4:5", "test_vkbin_unsorted.tsv") == 0)
    {
        (void) fprintf(stderr, "%s: an unsorted input was accepted\n", __func__);
        ++errors;
    }
    (void) remove("test_vkbin_unsorted.tsv");
    return errors;
}

int main(void)
{
    int errors = 0;

    errors += test_vkbin_args();
    errors += test_vkbin_errors();
    errors += test_vkbin_subset();

    // The error cases remove the output, so the good conversion runs last and
    // leaves the file that the remaining checks read.
    errors += test_vkbin_convert();
    if (errors > 0)
    {
        return errors;
    }

    mmfile_t mf = {0};
    mmap_binfile(VKBIN_OUT, &mf);
    if (mf.src == (uint8_t *)MAP_FAILED)
    {
        (void) fprintf(stderr, "main: cannot map '%s'\n", VKBIN_OUT);
        return 1;
    }

    errors += test_vkbin_header(mf);
    errors += test_vkbin_values(mf);
    errors += test_vkbin_sorted(mf);
    errors += test_vkbin_notfound(mf);

    int err = munmap_binfile(&mf);
    if (err != 0)
    {
        (void) fprintf(stderr, "main: got %d error while unmapping '%s'\n", err, VKBIN_OUT);
        ++errors;
    }
    (void) remove(VKBIN_OUT);

    return errors;
}
