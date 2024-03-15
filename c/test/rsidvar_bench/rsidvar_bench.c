// Benchmark tool for rsidvar
//
// rsidvar_bench.c
//
// @category   Tools
// @author     Nicola Asuni <info@tecnick.com>
// @license    MIT (see LICENSE)
// @link       https://github.com/tecnickcom/variantkey
//
// LICENSE
//
// Copyright (c) 2017-2018 GENOMICS plc
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// VariantKey by Nicola Asuni

// NOTE: This test is slow because it generates the test files from scratch.

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include "../../src/variantkey/rsidvar.h"

#define TEST_DATA_SIZE 10000000UL

// returns current time in nanoseconds
uint64_t get_time()
{
    struct timespec t;
    (void) timespec_get(&t, TIME_UTC);
    return (((uint64_t)t.tv_sec * 1000000000) + (uint64_t)t.tv_nsec);
}

int benchmark_find_rv_variantkey_by_rsid()
{
    const char *filename = "rsvk_test.bin";

    uint32_t i = 0;

    FILE *f = fopen(filename, "we");
    if (f == NULL)
    {
        (void) fprintf(stderr, " * %s Unable to open %s file in writing mode.\n", __func__, filename);
        return 1;
    }
    uint8_t b0 = 0, b1 = 0, b2 = 0, b3 = 0, z = 0;
    for (i=0 ; i < TEST_DATA_SIZE; i++)
    {
        b0 = i & 0xFF;
        b1 = (i >> 8) & 0xFF;
        b2 = (i >> 16) & 0xFF;
        b3 = (i >> 24) & 0xFF;
        (void) fprintf(f, "%c%c%c%c", b0, b1, b2, b3);
    }
    for (i=0 ; i < TEST_DATA_SIZE; i++)
    {
        b0 = i & 0xFF;
        b1 = (i >> 8) & 0xFF;
        b2 = (i >> 16) & 0xFF;
        b3 = (i >> 24) & 0xFF;
        (void) fprintf(f, "%c%c%c%c%c%c%c%c", b0, b1, b2, b3, z, z, z, z);
    }
    (void) fclose(f);

    mmfile_t rv = {0};
    rv.ncols = 2;
    rv.ctbytes[0] = 4;
    rv.ctbytes[1] = 8;
    rsidvar_cols_t crv = {0};
    mmap_rsvk_file(filename, &rv, &crv);
    if (crv.nrows != TEST_DATA_SIZE)
    {
        (void) fprintf(stderr, " * %s Expecting rsvk_test.bin %" PRIu64 " items, got instead: %" PRIu64 "\n", __func__, TEST_DATA_SIZE, crv.nrows);
        return 1;
    }

    uint64_t tstart = 0, tend = 0, offset = 0;
    volatile uint64_t sum = 0;
    uint64_t first = 0;

    tstart = get_time();
    for (i=0 ; i < TEST_DATA_SIZE; i++)
    {
        sum += i;
    }
    tend = get_time();
    offset = (tend - tstart);
    (void) fprintf(stdout, " * %s sum: %" PRIu64 "\n", __func__, sum);

    int j =0;
    for (j=0 ; j < 3; j++)
    {
        sum = 0;
        tstart = get_time();
        for (i=0 ; i < TEST_DATA_SIZE; i++)
        {
            first = 0;
            sum += find_rv_variantkey_by_rsid(crv, &first, crv.nrows, i);
        }
        tend = get_time();
        (void) fprintf(stdout, "   * %s %d. sum: %" PRIu64 " -- time: %" PRIu64 " ns -- %" PRIu64 " ns/op\n", __func__, j, sum, (tend - tstart - offset), (tend - tstart - offset)/TEST_DATA_SIZE);
    }
    return 0;
}

int benchmark_find_vr_rsid_by_variantkey()
{
    const char *filename = "vkrs_test.bin";

    uint64_t i = 0;

    FILE *f = fopen(filename, "we");
    if (f == NULL)
    {
        (void) fprintf(stderr, " * %s Unable to open %s file in writing mode.\n", __func__, filename);
        return 1;
    }
    uint8_t b0 = 0, b1 = 0, b2 = 0, b3 = 0, z = 0;
    for (i=0 ; i < TEST_DATA_SIZE; i++)
    {
        b0 = i & 0xFF;
        b1 = (i >> 8) & 0xFF;
        b2 = (i >> 16) & 0xFF;
        b3 = (i >> 24) & 0xFF;
        (void) fprintf(f, "%c%c%c%c%c%c%c%c", b0, b1, b2, b3, z, z, z, z);
    }
    for (i=0 ; i < TEST_DATA_SIZE; i++)
    {
        b0 = i & 0xFF;
        b1 = (i >> 8) & 0xFF;
        b2 = (i >> 16) & 0xFF;
        b3 = (i >> 24) & 0xFF;
        (void) fprintf(f, "%c%c%c%c", b0, b1, b2, b3);
    }
    (void) fclose(f);

    mmfile_t vr = {0};
    vr.ncols = 2;
    vr.ctbytes[0] = 8;
    vr.ctbytes[1] = 4;
    rsidvar_cols_t cvr = {0};
    mmap_vkrs_file(filename, &vr, &cvr);
    if (cvr.nrows != TEST_DATA_SIZE)
    {
        (void) fprintf(stderr, " * %s Expecting vkrs_test_400M.bin %" PRIu64 " items, got instead: %" PRIu64 "\n", __func__, TEST_DATA_SIZE, cvr.nrows);
        return 1;
    }

    uint64_t tstart = 0, tend = 0, offset = 0;
    volatile uint64_t sum = 0;
    uint64_t first = 0;

    tstart = get_time();
    for (i=0 ; i < TEST_DATA_SIZE; i++)
    {
        sum += i;
    }
    tend = get_time();
    offset = (tend - tstart);
    (void) fprintf(stdout, " * %s sum: %" PRIu64 "\n", __func__, sum);

    int j = 0;
    for (j=0 ; j < 3; j++)
    {
        sum = 0;
        tstart = get_time();
        for (i=0 ; i < TEST_DATA_SIZE; i++)
        {
            first = 0;
            sum += find_vr_rsid_by_variantkey(cvr, &first, cvr.nrows, i);
        }
        tend = get_time();
        (void) fprintf(stdout, "   * %s %d. sum: %" PRIu64 " -- time: %" PRIu64 " ns -- %" PRIu64 " ns/op\n", __func__, j, sum, (tend - tstart - offset), (tend - tstart - offset)/TEST_DATA_SIZE);
    }
    return 0;
}

int main()
{
    int ret = 0;
    ret += benchmark_find_rv_variantkey_by_rsid();
    ret += benchmark_find_vr_rsid_by_variantkey();
    return ret;
}
