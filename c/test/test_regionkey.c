// VariantKey
//
// test_regionkey.c
//
// @category   Test
// @author     Nicola Asuni <info@tecnick.com>
// @link       https://github.com/tecnickcom/variantkey
// @license    MIT [LICENSE](https://raw.githubusercontent.com/tecnickcom/variantkey/main/LICENSE)

// Test for regionkey

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <sys/mman.h>
#include "../src/variantkey/regionkey.h"

#define TEST_DATA_SIZE 10
#define TEST_OVERLAP_SIZE 12

typedef struct test_data_t
{
    const char* chrom;
    uint32_t    startpos;
    uint32_t    endpos;
    int8_t      strand;
    uint8_t     echrom;
    uint8_t     estrand;
    uint64_t    rk;
    const char* rs;
    uint64_t    chrom_startpos;
    uint64_t    chrom_endpos;
} __attribute__((packed)) __attribute__((aligned(128))) test_data_t;

static const test_data_t test_data[TEST_DATA_SIZE] =
{
    { "1", 1000, 1100,  0,  1, 0, 0x080001f400002260, "080001f400002260", 0x00000000100003e8, 0x000000001000044c},
    { "2", 1001, 1201,  1,  2, 1, 0x100001f48000258a, "100001f48000258a", 0x00000000200003e9, 0x00000000200004b1},
    { "3", 1002, 1302, -1,  3, 2, 0x180001f5000028b4, "180001f5000028b4", 0x00000000300003ea, 0x0000000030000516},
    { "4", 1003, 1403,  0,  4, 0, 0x200001f580002bd8, "200001f580002bd8", 0x00000000400003eb, 0x000000004000057b},
    { "5", 1004, 1504,  1,  5, 1, 0x280001f600002f02, "280001f600002f02", 0x00000000500003ec, 0x00000000500005e0},
    {"10", 1005, 1605, -1, 10, 2, 0x500001f68000322c, "500001f68000322c", 0x00000000a00003ed, 0x00000000a0000645},
    {"22", 1006, 1706,  0, 22, 0, 0xb00001f700003550, "b00001f700003550", 0x00000001600003ee, 0x00000001600006aa},
    { "X", 1007, 1807,  1, 23, 1, 0xb80001f78000387a, "b80001f78000387a", 0x00000001700003ef, 0x000000017000070f},
    { "Y", 1008, 1908, -1, 24, 2, 0xc00001f800003ba4, "c00001f800003ba4", 0x00000001800003f0, 0x0000000180000774},
    {"MT", 1009, 2009,  0, 25, 0, 0xc80001f880003ec8, "c80001f880003ec8", 0x00000001900003f1, 0x00000001900007d9},
};

typedef struct test_overlap_t
{
    uint8_t  res;
    uint8_t  a_chrom;
    uint8_t  b_chrom;
    uint32_t a_startpos;
    uint32_t b_startpos;
    uint32_t a_endpos;
    uint32_t b_endpos;
    uint64_t a_rk;
    uint64_t a_vk;
    uint64_t b_rk;
} __attribute__((packed)) __attribute__((aligned(128))) test_overlap_t;

static const test_overlap_t test_overlap[TEST_OVERLAP_SIZE] =
{
    {0,  1,  2, 5, 5,  7, 7, 0x0800000280000038, 0x0800000290920000, 0x1000000280000038}, // different chromosome
    {0,  1,  1, 0, 3,  2, 7, 0x0800000000000010, 0x0800000010920000, 0x0800000180000038}, // -[-]|---|----
    {0,  2,  2, 1, 3,  3, 7, 0x1000000080000018, 0x1000000090920000, 0x1000000180000038}, // --[-]---|----
    {1,  3,  3, 2, 3,  4, 7, 0x1800000100000020, 0x1800000110920000, 0x1800000180000038}, // ---[|]--|----
    {1,  4,  4, 3, 3,  5, 7, 0x2000000180000028, 0x2000000190920000, 0x2000000180000038}, // ----[-]-|----
    {1,  5,  5, 4, 3,  6, 7, 0x2800000200000030, 0x2800000210920000, 0x2800000180000038}, // ----|[-]|----
    {1,  6,  6, 5, 3,  7, 7, 0x3000000280000038, 0x3000000290920000, 0x3000000180000038}, // ----|-[ ]----
    {1, 10, 10, 6, 3,  8, 7, 0x5000000300000040, 0x5000000310920000, 0x5000000180000038}, // ----|--[|]---
    {0, 22, 22, 7, 3,  9, 7, 0xb000000380000048, 0xb000000390920000, 0xb000000180000038}, // ----|---[-]--
    {0, 23, 23, 8, 3, 10, 7, 0xb800000400000050, 0xb800000410920000, 0xb800000180000038}, // ----|---|[-]-
    {1, 24, 24, 2, 3,  8, 7, 0xc000000100000040, 0xc000000130911200, 0xc000000180000038}, // ---[|---|]---
    {1, 25, 25, 3, 3,  7, 7, 0xc800000180000038, 0xc8000001a0912000, 0xc800000180000038}, // ----[---]----
};

// returns current time in nanoseconds
uint64_t get_time()
{
    struct timespec t;
    (void) timespec_get(&t, TIME_UTC);
    return (((uint64_t)t.tv_sec * 1000000000) + (uint64_t)t.tv_nsec);
}

int test_encode_region_strand()
{
    int errors = 0;
    int i = 0;
    uint8_t res = 0;
    for (i=0 ; i < TEST_DATA_SIZE; i++)
    {
        res = encode_region_strand(test_data[i].strand);
        if (res != test_data[i].estrand)
        {
            (void) fprintf(stderr, "%s (%d) Expecting encoded STRAND %d, got %d\n", __func__, i, test_data[i].estrand, res);
            ++errors;
        }
    }
    return errors;
}

int test_decode_region_strand()
{
    int errors = 0;
    int i = 0;
    int8_t res = 0;
    for (i=0 ; i < TEST_DATA_SIZE; i++)
    {
        res = decode_region_strand(test_data[i].estrand);
        if (res != test_data[i].strand)
        {
            (void) fprintf(stderr, "%s (%d) Expecting STRAND %d, got %d\n", __func__, i, test_data[i].strand, res);
            ++errors;
        }
    }
    return errors;
}

int test_encode_regionkey()
{
    int errors = 0;
    int i = 0;
    uint64_t res = 0;
    for (i=0 ; i < TEST_DATA_SIZE; i++)
    {
        res = encode_regionkey(test_data[i].echrom, test_data[i].startpos, test_data[i].endpos, test_data[i].estrand);
        if (res != test_data[i].rk)
        {
            (void) fprintf(stderr, "%s (%d) Expecting RegionKey %016" PRIx64 ", got %016" PRIx64 "\n", __func__, i, test_data[i].rk, res);
            ++errors;
        }
    }
    return errors;
}

int test_extract_regionkey_chrom()
{
    int errors = 0;
    int i = 0;
    uint8_t res = 0;
    for (i=0 ; i < TEST_DATA_SIZE; i++)
    {
        res = extract_regionkey_chrom(test_data[i].rk);
        if (res != test_data[i].echrom)
        {
            (void) fprintf(stderr, "%s (%d) Expecting CHROM %d, got %d\n", __func__, i, test_data[i].echrom, res);
            ++errors;
        }
    }
    return errors;
}

int test_extract_regionkey_startpos()
{
    int errors = 0;
    int i = 0;
    uint32_t res = 0;
    for (i=0 ; i < TEST_DATA_SIZE; i++)
    {
        res = extract_regionkey_startpos(test_data[i].rk);
        if (res != test_data[i].startpos)
        {
            (void) fprintf(stderr, "%s (%d) Expecting CHROM %" PRIu32 ", got %" PRIu32 "\n", __func__, i, test_data[i].startpos, res);
            ++errors;
        }
    }
    return errors;
}

int test_extract_regionkey_endpos()
{
    int errors = 0;
    int i = 0;
    uint32_t res = 0;
    for (i=0 ; i < TEST_DATA_SIZE; i++)
    {
        res = extract_regionkey_endpos(test_data[i].rk);
        if (res != test_data[i].endpos)
        {
            (void) fprintf(stderr, "%s (%d) Expecting CHROM %" PRIu32 ", got %" PRIu32 "\n", __func__, i, test_data[i].endpos, res);
            ++errors;
        }
    }
    return errors;
}

int test_extract_regionkey_strand()
{
    int errors = 0;
    int i = 0;
    uint8_t res = 0;
    for (i=0 ; i < TEST_DATA_SIZE; i++)
    {
        res = extract_regionkey_strand(test_data[i].rk);
        if (res != test_data[i].estrand)
        {
            (void) fprintf(stderr, "%s (%d) Expecting CHROM %" PRIu32 ", got %" PRIu32 "\n", __func__, i, test_data[i].estrand, res);
            ++errors;
        }
    }
    return errors;
}

int test_decode_regionkey()
{
    int errors = 0;
    int i = 0;
    regionkey_t h = {0};
    for (i=0 ; i < TEST_DATA_SIZE; i++)
    {
        decode_regionkey(test_data[i].rk, &h);
        if (h.chrom != test_data[i].echrom)
        {
            (void) fprintf(stderr, "%s (%d): Unexpected CHROM code: expected %" PRIu8 ", got %" PRIu8 "\n", __func__, i, test_data[i].echrom, h.chrom);
            ++errors;
        }
        if (h.startpos != test_data[i].startpos)
        {
            (void) fprintf(stderr, "%s (%d): Unexpected START POS: expected %" PRIu32 ", got %" PRIu32 "\n", __func__, i, test_data[i].startpos, h.startpos);
            ++errors;
        }
        if (h.endpos != test_data[i].endpos)
        {
            (void) fprintf(stderr, "%s (%d): Unexpected END POS: expected %" PRIu32 ", got %" PRIu32 "\n", __func__, i, test_data[i].endpos, h.endpos);
            ++errors;
        }
        if (h.strand != test_data[i].estrand)
        {
            (void) fprintf(stderr, "%s (%d): Unexpected CHROM code: expected %" PRIu8 ", got %" PRIu8 "\n", __func__, i, test_data[i].estrand, h.strand);
            ++errors;
        }
    }
    return errors;
}

void benchmark_decode_regionkey()
{
    regionkey_t h = {0};
    uint64_t tstart = 0, tend = 0;
    int i = 0;
    int size = 100000;
    tstart = get_time();
    for (i=0 ; i < size; i++)
    {
        decode_regionkey(0x080001f400002260 + i, &h);
    }
    tend = get_time();
    (void) fprintf(stdout, " * %s : %lu ns/op\n", __func__, (tend - tstart)/size);
}

int test_reverse_regionkey()
{
    int errors = 0;
    int i = 0;
    regionkey_rev_t h = {0};
    for (i=0 ; i < TEST_DATA_SIZE; i++)
    {
        reverse_regionkey(test_data[i].rk, &h);
        if (strcasecmp(test_data[i].chrom, h.chrom) != 0)
        {
            (void) fprintf(stderr, "%s (%d): Unexpected CHROM: expected %s, got %s\n", __func__, i, test_data[i].chrom, h.chrom);
            ++errors;
        }
        if (h.startpos != test_data[i].startpos)
        {
            (void) fprintf(stderr, "%s (%d): Unexpected START POS: expected %" PRIu32 ", got %" PRIu32 "\n", __func__, i, test_data[i].startpos, h.startpos);
            ++errors;
        }
        if (h.endpos != test_data[i].endpos)
        {
            (void) fprintf(stderr, "%s (%d): Unexpected END POS: expected %" PRIu32 ", got %" PRIu32 "\n", __func__, i, test_data[i].endpos, h.endpos);
            ++errors;
        }
        if (h.strand != test_data[i].strand)
        {
            (void) fprintf(stderr, "%s (%d): Unexpected CHROM code: expected %d, got %d\n", __func__, i, test_data[i].strand, h.strand);
            ++errors;
        }
    }
    return errors;
}

void benchmark_reverse_regionkey()
{
    regionkey_rev_t h = {0};
    uint64_t tstart = 0, tend = 0;
    int i = 0;
    int size = 100000;
    tstart = get_time();
    for (i=0 ; i < size; i++)
    {
        reverse_regionkey(0x080001f400002260 + i, &h);
    }
    tend = get_time();
    (void) fprintf(stdout, " * %s : %lu ns/op\n", __func__, (tend - tstart)/size);
}

int test_regionkey()
{
    int errors = 0;
    int i = 0;
    uint64_t res = 0;
    for (i=0 ; i < TEST_DATA_SIZE; i++)
    {
        res = regionkey(test_data[i].chrom, strlen(test_data[i].chrom), test_data[i].startpos, test_data[i].endpos, test_data[i].strand);
        if (res != test_data[i].rk)
        {
            (void) fprintf(stderr, "%s (%d) Expecting RegionKey %016" PRIx64 ", got %016" PRIx64 "\n", __func__, i, test_data[i].rk, res);
            ++errors;
        }
    }
    return errors;
}

void benchmark_regionkey()
{
    uint64_t res = 0;
    uint64_t tstart = 0, tend = 0;
    int i = 0;
    int size = 100000;
    tstart = get_time();
    for (i=0 ; i < size; i++)
    {
        res = regionkey("MT", 2, 1000, 2000, -1);
    }
    tend = get_time();
    (void) fprintf(stdout, " * %s : %lu ns/op (%" PRIx64 ")\n", __func__, (tend - tstart)/size, res);
}

int test_extend_regionkey()
{
    int errors = 0;
    uint64_t rk = regionkey("X", 1, 10000, 20000, -1);
    uint64_t erk = extend_regionkey(rk, 1000);
    uint32_t startpos = extract_regionkey_startpos(erk);
    uint32_t endpos = extract_regionkey_endpos(erk);
    if (startpos != 9000)
    {
        (void) fprintf(stderr, "%s Expecting STARTPOS 9000, got %" PRIu32 "\n", __func__, startpos);
        ++errors;
    }
    if (endpos != 21000)
    {
        (void) fprintf(stderr, "%s Expecting ENDPOS 21000, got %" PRIu32 "\n", __func__, endpos);
        ++errors;
    }
    erk = extend_regionkey(rk, 300000000);
    startpos = extract_regionkey_startpos(erk);
    endpos = extract_regionkey_endpos(erk);
    if (startpos != 0)
    {
        (void) fprintf(stderr, "%s Expecting STARTPOS 9000, got %" PRIu32 "\n", __func__, startpos);
        ++errors;
    }
    if (endpos != RK_MAX_POS)
    {
        (void) fprintf(stderr, "%s Expecting ENDPOS %" PRIu32 ", got %" PRIu32 "\n", __func__, RK_MAX_POS, endpos);
        ++errors;
    }
    return errors;
}

int test_regionkey_hex()
{
    int errors = 0;
    int i = 0;
    char rs[17] = "";
    for (i=0 ; i < TEST_DATA_SIZE; i++)
    {
        regionkey_hex(test_data[i].rk, rs);
        if (strcmp(rs, test_data[i].rs) != 0)
        {
            (void) fprintf(stderr, "%s (%d) Expecting RegionKey HEX %s, got %s\n", __func__, i, test_data[i].rs, rs);
            ++errors;
        }
    }
    return errors;
}

int test_parse_regionkey_hex()
{
    int errors = 0;
    int i = 0;
    uint64_t rk = 0;
    rk = parse_regionkey_hex("1234567890AbCdEf");
    if (rk != 0x1234567890abcdef)
    {
        (void) fprintf(stderr, "%s : Unexpected regionkey: expected 0x1234567890abcdef, got 0x%016" PRIx64 "\n", __func__, rk);
        ++errors;
    }
    for (i=0 ; i < TEST_DATA_SIZE; i++)
    {
        rk = parse_variantkey_hex(test_data[i].rs);
        if (rk != test_data[i].rk)
        {
            (void) fprintf(stderr, "%s (%d): Unexpected regionkey: expected 0x%016" PRIx64 ", got 0x%016" PRIx64 "\n", __func__, i, test_data[i].rk, rk);
            ++errors;
        }
    }
    return errors;
}

int test_get_regionkey_chrom_startpos()
{
    int errors = 0;
    int i = 0;
    uint64_t res = 0;
    for (i=0 ; i < TEST_DATA_SIZE; i++)
    {
        res = get_regionkey_chrom_startpos(test_data[i].rk);
        if (res != test_data[i].chrom_startpos)
        {
            (void) fprintf(stderr, "%s (%d) Expecting CHROM + START POS %016" PRIx64 ", got %016" PRIx64 "\n", __func__, i, test_data[i].chrom_startpos, res);
            ++errors;
        }
    }
    return errors;
}

int test_get_regionkey_chrom_endpos()
{
    int errors = 0;
    int i = 0;
    uint64_t res = 0;
    for (i=0 ; i < TEST_DATA_SIZE; i++)
    {
        res = get_regionkey_chrom_endpos(test_data[i].rk);
        if (res != test_data[i].chrom_endpos)
        {
            (void) fprintf(stderr, "%s (%d) Expecting CHROM + END POS %016" PRIx64 ", got %016" PRIx64 "\n", __func__, i, test_data[i].chrom_endpos, res);
            ++errors;
        }
    }
    return errors;
}

int test_are_overlapping_regions()
{
    int errors = 0;
    int i = 0;
    uint8_t res = 0;
    for (i=0 ; i < TEST_OVERLAP_SIZE; i++)
    {
        res = are_overlapping_regions(test_overlap[i].a_chrom, test_overlap[i].a_startpos, test_overlap[i].a_endpos, test_overlap[i].b_chrom, test_overlap[i].b_startpos, test_overlap[i].b_endpos);
        if (res != test_overlap[i].res)
        {
            (void) fprintf(stderr, "%s (%d) Expecting %" PRIu8 ", got %" PRIu8 "\n", __func__, i, test_overlap[i].res, res);
            ++errors;
        }
    }
    return errors;
}

int test_are_overlapping_region_regionkey()
{
    int errors = 0;
    int i = 0;
    uint8_t res = 0;
    for (i=0 ; i < TEST_OVERLAP_SIZE; i++)
    {
        res = are_overlapping_region_regionkey(test_overlap[i].a_chrom, test_overlap[i].a_startpos, test_overlap[i].a_endpos, test_overlap[i].b_rk);
        if (res != test_overlap[i].res)
        {
            (void) fprintf(stderr, "%s (%d) Expecting %" PRIu8 ", got %" PRIu8 "\n", __func__, i, test_overlap[i].res, res);
            ++errors;
        }
    }
    return errors;
}

int test_are_overlapping_regionkeys()
{
    int errors = 0;
    int i = 0;
    uint8_t res = 0;
    for (i=0 ; i < TEST_OVERLAP_SIZE; i++)
    {
        res = are_overlapping_regionkeys(test_overlap[i].a_rk, test_overlap[i].b_rk);
        if (res != test_overlap[i].res)
        {
            (void) fprintf(stderr, "%s (%d) Expecting %" PRIu8 ", got %" PRIu8 "\n", __func__, i, test_overlap[i].res, res);
            ++errors;
        }
    }
    return errors;
}

int test_are_overlapping_variantkey_regionkey(nrvk_cols_t nvc)
{
    int errors = 0;
    int i = 0;
    uint8_t res = 0;
    for (i=0 ; i < TEST_OVERLAP_SIZE; i++)
    {
        res = are_overlapping_variantkey_regionkey(nvc, test_overlap[i].a_vk, test_overlap[i].b_rk);
        if (res != test_overlap[i].res)
        {
            (void) fprintf(stderr, "%s (%d) Expecting %" PRIu8 ", got %" PRIu8 "\n", __func__, i, test_overlap[i].res, res);
            ++errors;
        }
    }
    return errors;
}

int test_variantkey_to_regionkey(nrvk_cols_t nvc)
{
    int errors = 0;
    int i = 0;
    uint64_t res = 0;
    for (i=0 ; i < TEST_OVERLAP_SIZE; i++)
    {
        res = variantkey_to_regionkey(nvc, test_overlap[i].a_vk);
        if (res != test_overlap[i].a_rk)
        {
            (void) fprintf(stderr, "%s (%d) Expecting %016" PRIx64 ", got %016" PRIx64 "\n", __func__, i, test_overlap[i].a_rk, res);
            ++errors;
        }
    }
    return errors;
}

int main()
{
    int errors = 0;
    int err = 0;

    mmfile_t nrvk = {0};
    nrvk_cols_t nvc = {0};
    mmap_nrvk_file("nrvk.10.bin", &nrvk, &nvc);

    if (nrvk.nrows != TEST_DATA_SIZE)
    {
        (void) fprintf(stderr, "Expecting %d items, got instead: %" PRIu64 "\n", TEST_DATA_SIZE, nrvk.nrows);
        return 1;
    }

    errors += test_encode_region_strand();
    errors += test_decode_region_strand();
    errors += test_encode_regionkey();
    errors += test_extract_regionkey_chrom();
    errors += test_extract_regionkey_startpos();
    errors += test_extract_regionkey_endpos();
    errors += test_extract_regionkey_strand();
    errors += test_decode_regionkey();
    errors += test_reverse_regionkey();
    errors += test_regionkey();
    errors += test_extend_regionkey();
    errors += test_regionkey_hex();
    errors += test_parse_regionkey_hex();
    errors += test_get_regionkey_chrom_startpos();
    errors += test_get_regionkey_chrom_endpos();
    errors += test_are_overlapping_regions();
    errors += test_are_overlapping_region_regionkey();
    errors += test_are_overlapping_regionkeys();
    errors += test_are_overlapping_variantkey_regionkey(nvc);
    errors += test_variantkey_to_regionkey(nvc);

    benchmark_decode_regionkey();
    benchmark_reverse_regionkey();
    benchmark_regionkey();

    err = munmap_binfile(nrvk);
    if (err != 0)
    {
        (void) fprintf(stderr, "Got %d error while unmapping the nrvk file\n", err);
        return 1;
    }

    return errors;
}
