// BinSearch
//
// test_binsearch_mmap.c
//
// @category   Test
// @author     Nicola Asuni <info@tecnick.com>
// @link       https://github.com/tecnickcom/binsearch
// @license    MIT (see LICENSE file)
// @copyright  (c) 2017-2026 Nicola Asuni - Tecnick.com

// Tests for the file mapping and header parsing error paths.

// Annex K is optional: __STDC_LIB_EXT1__ is only defined by the standard
// headers, so the request must come before the first include and the fallback
// after it.
#define __STDC_WANT_LIB_EXT1__ 1

#include <stdio.h>
#ifndef __STDC_LIB_EXT1__
#define fprintf_s fprintf
#endif
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

// The fstat and mmap failure paths cannot be reached with a real file, so both
// calls are redirected to a wrapper that fails on demand. The wrappers are
// plain functions and the macros are object-like, so the call sites in
// binsearch.h keep exactly the same shape and the coverage data of this
// translation unit merges with the others.
static int stub_fstat_fail = 0;
static int stub_mmap_fail = 0;

static int stub_fstat_call(int fd, struct stat *buf)
{
    if (stub_fstat_fail)
    {
        return -1;
    }
    return fstat(fd, buf);
}

static void *stub_mmap_call(void *addr, size_t len, int prot, int flags, int fd, off_t off)
{
    if (stub_mmap_fail)
    {
        return MAP_FAILED;
    }
    return mmap(addr, len, prot, flags, fd, off);
}

#define fstat stub_fstat_call
#define mmap stub_mmap_call

#include "../src/variantkey/binsearch.h"

#define TEST_FILE "test_tmp_mmap.bin" //!< Name of the temporary file used by the tests

// Writes the given bytes to TEST_FILE and returns 0 on success.
static int write_test_file(const uint8_t *data, size_t size)
{
    FILE *fp = fopen(TEST_FILE, "wb");
    if (fp == NULL)
    {
        (void)fprintf_s(stderr, "%s unable to create %s\n", __func__, TEST_FILE);
        return 1;
    }
    size_t written = fwrite(data, 1, size, fp);
    if (fclose(fp) != 0 || written != size)
    {
        (void)fprintf_s(stderr, "%s unable to write %s\n", __func__, TEST_FILE);
        return 1;
    }
    return 0;
}

// Stores a 64 bit value in little-endian order.
static void set_uint64_le(uint8_t *data, uint64_t value)
{
    for (size_t i = 0; i < 8; i++)
    {
        data[i] = (uint8_t)(value >> (i * 8));
    }
}

// Maps TEST_FILE, checks the resulting data block and unmaps it.
// The ctbytes array must have at least ncols items.
static int check_test_file(const char *name, const uint8_t *data, size_t size,
                           uint8_t ncols, const uint8_t *ctbytes,
                           uint64_t doffset, uint64_t dlength, uint64_t nrows)
{
    if (write_test_file(data, size) != 0)
    {
        return 1;
    }
    int errors = 0;
    mmfile_t mf = {0};
    mf.ncols = ncols;
    for (uint8_t i = 0; i < ncols; i++)
    {
        mf.ctbytes[i] = ctbytes[i];
    }
    mmap_binfile(TEST_FILE, &mf);
    if (mf.src == MAP_FAILED)
    {
        (void)fprintf_s(stderr, "%s (%s) unable to map the file\n", __func__, name);
        (void)remove(TEST_FILE);
        return 1;
    }
    if (mf.doffset != doffset)
    {
        (void)fprintf_s(stderr, "%s (%s) mf.doffset : Expecting %" PRIu64 ", got instead: %" PRIu64 "\n",
                        __func__, name, doffset, mf.doffset);
        errors++;
    }
    if (mf.dlength != dlength)
    {
        (void)fprintf_s(stderr, "%s (%s) mf.dlength : Expecting %" PRIu64 ", got instead: %" PRIu64 "\n",
                        __func__, name, dlength, mf.dlength);
        errors++;
    }
    if (mf.nrows != nrows)
    {
        (void)fprintf_s(stderr, "%s (%s) mf.nrows : Expecting %" PRIu64 ", got instead: %" PRIu64 "\n",
                        __func__, name, nrows, mf.nrows);
        errors++;
    }
    if (munmap_binfile(mf) != 0)
    {
        (void)fprintf_s(stderr, "%s (%s) error while unmapping the file\n", __func__, name);
        errors++;
    }
    (void)remove(TEST_FILE);
    return errors;
}

// Maps a file of the given size and checks that the mapping failed.
static int check_map_failure(const char *name, const uint8_t *data, size_t size)
{
    if (write_test_file(data, size) != 0)
    {
        return 1;
    }
    int errors = 0;
    mmfile_t mf = {0};
    mmap_binfile(TEST_FILE, &mf);
    if (mf.src != MAP_FAILED)
    {
        (void)fprintf_s(stderr, "%s (%s) an mmap error was expected\n", __func__, name);
        (void)munmap_binfile(mf);
        errors++;
    }
    if (mf.fd >= 0)
    {
        (void)fprintf_s(stderr, "%s (%s) the file descriptor was not closed\n", __func__, name);
        errors++;
    }
    (void)remove(TEST_FILE);
    return errors;
}

// Column types used by the tests that do not declare their own.
static const uint8_t ctbytes_none[1] = {0};
static const uint8_t ctbytes_4_8[2] = {4, 8};

// A 28-byte file with an unknown magic number and no column type.
static int test_unknown_format(void)
{
    uint8_t data[28] = {0};
    return check_test_file("unknown", data, sizeof(data), 0, ctbytes_none, 0, 28, 0);
}

// A file shorter than the minimum header size is mapped but not parsed.
static int test_short_file(void)
{
    uint8_t data[10] = {0};
    return check_test_file("short", data, sizeof(data), 0, ctbytes_none, 0, 10, 0);
}

// The columns of a file shorter than the minimum header size are still described
// by the fields set by the caller.
static int test_short_file_columns(void)
{
    uint8_t data[24] = {0};
    return check_test_file("short with columns", data, sizeof(data), 2, ctbytes_4_8, 0, 24, 2);
}

// Column types that do not fit the file: the third column ends past the end.
// Only one row fits once the 8-byte padding between the columns is taken into
// account (1 + 7 padding, 1 + 7 padding, 8 = 24 bytes of the 28 available).
static int test_columns_out_of_range(void)
{
    static const uint8_t ctbytes[3] = {1, 1, 8};
    uint8_t data[28] = {0};
    return check_test_file("columns out of range", data, sizeof(data), 3, ctbytes, 0, 28, 1);
}

// Two columns padded to an 8-byte boundary: the padding bytes belong to the
// data block but hold no value, so the number of rows must be derived from the
// padded layout and not from the sum of the column type sizes alone.
// The layout of 100 rows of two uint8_t columns is 100 + 4 padding + 100.
static int test_columns_padded(void)
{
    static const uint8_t ctbytes[2] = {1, 1};
    uint8_t data[204] = {0};
    return check_test_file("columns padded", data, sizeof(data), 2, ctbytes, 0, 204, 100);
}

// Three columns padded to an 8-byte boundary in a data block that cannot hold
// a single row: one row needs 8 + 8 + 1 bytes and only 16 are available, even
// though the sum of the column type sizes would suggest five rows.
static int test_columns_no_room(void)
{
    static const uint8_t ctbytes[3] = {1, 1, 1};
    uint8_t data[16] = {0};
    return check_test_file("columns no room", data, sizeof(data), 3, ctbytes, 0, 16, 0);
}

// A column type size that is not a power of two cannot be read through a typed
// pointer, so the data block is rejected.
static int test_columns_odd_type(void)
{
    static const uint8_t ctbytes[1] = {3};
    uint8_t data[28] = {0};
    return check_test_file("columns odd type", data, sizeof(data), 1, ctbytes, 0, 0, 0);
}

// The data block of an ARROW1 file without a footer starts at byte 17, so an
// 8-byte column would be reported at a misaligned offset and is rejected.
static int test_columns_misaligned(void)
{
    static const uint8_t ctbytes[1] = {8};
    uint8_t data[28] = {0};
    memcpy(data, "ARROW1", 6);
    return check_test_file("columns misaligned", data, sizeof(data), 1, ctbytes, 0, 0, 0);
}

// A BINSRC1 header that declares more columns than the file can hold.
static int test_binsrc_truncated(void)
{
    uint8_t data[28] = {0};
    memcpy(data, "BINSRC1", 7);
    data[8] = 255; // number of columns
    return check_test_file("binsrc truncated", data, sizeof(data), 0, ctbytes_none, 0, 0, 0);
}

// Fills a BINSRC1 file of at least 40 bytes with one column.
// The header is: magic number, number of columns, one column type, padding,
// number of rows and one column offset; the data block starts at byte 32.
static void fill_binsrc(uint8_t *data, uint8_t ctbytes, uint64_t nrows, uint64_t index)
{
    memcpy(data, "BINSRC1", 7);
    data[8] = 1; // number of columns
    data[9] = ctbytes;
    set_uint64_le((data + 16), nrows);
    set_uint64_le((data + 24), index);
}

// A BINSRC1 header that declares no column: the number of rows that follows it
// cannot describe anything and must not be reported.
static int test_binsrc_no_columns(void)
{
    uint8_t data[40] = {0};
    fill_binsrc(data, 8, 1, 32);
    data[8] = 0; // number of columns
    return check_test_file("binsrc no columns", data, sizeof(data), 0, ctbytes_none, 0, 0, 0);
}

// A BINSRC1 header with a column type size that is not a power of two.
static int test_binsrc_odd_type(void)
{
    uint8_t data[40] = {0};
    fill_binsrc(data, 3, 1, 32);
    return check_test_file("binsrc odd type", data, sizeof(data), 0, ctbytes_none, 0, 0, 0);
}

// A BINSRC1 header with a column type larger than the widest supported one.
static int test_binsrc_wide_type(void)
{
    uint8_t data[40] = {0};
    fill_binsrc(data, 16, 1, 32);
    return check_test_file("binsrc wide type", data, sizeof(data), 0, ctbytes_none, 0, 0, 0);
}

// A BINSRC1 header with a column offset that is not a multiple of the column
// type size: the column mode functions would read it through a misaligned
// typed pointer. The column fits in the file, so only the alignment is wrong.
static int test_binsrc_misaligned_index(void)
{
    uint8_t data[48] = {0};
    fill_binsrc(data, 8, 1, 33);
    return check_test_file("binsrc misaligned index", data, sizeof(data), 0, ctbytes_none, 0, 0, 0);
}

// A valid BINSRC1 file.
static int test_binsrc_valid(void)
{
    uint8_t data[40] = {0};
    fill_binsrc(data, 8, 1, 32);
    return check_test_file("binsrc valid", data, sizeof(data), 0, ctbytes_none, 32, 8, 1);
}

// A BINSRC1 header with a column type size of zero.
static int test_binsrc_zero_type(void)
{
    uint8_t data[40] = {0};
    fill_binsrc(data, 0, 1, 32);
    return check_test_file("binsrc zero type", data, sizeof(data), 0, ctbytes_none, 0, 0, 0);
}

// A BINSRC1 header with a column offset past the end of the file.
static int test_binsrc_bad_index(void)
{
    uint8_t data[40] = {0};
    fill_binsrc(data, 8, 1, 1000);
    return check_test_file("binsrc bad index", data, sizeof(data), 0, ctbytes_none, 0, 0, 0);
}

// A BINSRC1 header with more rows than the file can hold.
static int test_binsrc_bad_nrows(void)
{
    uint8_t data[40] = {0};
    fill_binsrc(data, 8, 100, 32);
    return check_test_file("binsrc bad nrows", data, sizeof(data), 0, ctbytes_none, 0, 0, 0);
}

// An ARROW1 header with a metadata length that runs past the end of the file.
static int test_arrow_bad_metadata(void)
{
    uint8_t data[28] = {0};
    memcpy(data, "ARROW1", 6);
    data[9] = 0xf0;
    data[10] = 0xff;
    data[11] = 0xff;
    data[12] = 0xff;
    return check_test_file("arrow bad metadata", data, sizeof(data), 2, ctbytes_4_8, 0, 0, 0);
}

// An ARROW1 header with a dictionary length that runs past the end of the file.
static int test_arrow_bad_dictionary(void)
{
    uint8_t data[28] = {0};
    memcpy(data, "ARROW1", 6);
    data[13] = 0xf0;
    data[14] = 0xff;
    data[15] = 0xff;
    data[16] = 0xff;
    return check_test_file("arrow bad dictionary", data, sizeof(data), 2, ctbytes_4_8, 0, 0, 0);
}

// An ARROW1 file with a footer larger than the data block.
static int test_arrow_bad_footer(void)
{
    uint8_t data[28] = {0};
    memcpy(data, "ARROW1", 6);
    data[18] = 100; // footer length
    memcpy(data + 22, "ARROW1", 6);
    return check_test_file("arrow bad footer", data, sizeof(data), 2, ctbytes_4_8, 0, 0, 0);
}

// An ARROW1 file without the trailing magic number keeps the whole data block.
// The data block of this file starts at byte 17, so it can only hold a 1-byte
// column: a wider one would be reported at a misaligned offset and rejected.
static int test_arrow_no_footer(void)
{
    static const uint8_t ctbytes[1] = {1};
    uint8_t data[28] = {0};
    memcpy(data, "ARROW1", 6);
    return check_test_file("arrow no footer", data, sizeof(data), 1, ctbytes, 17, 11, 11);
}

// A FEA1 file with a metadata block larger than the data block.
static int test_feather_bad_metadata(void)
{
    uint8_t data[28] = {0};
    memcpy(data, "FEA1", 4);
    data[20] = 100; // metadata length
    memcpy(data + 24, "FEA1", 4);
    return check_test_file("feather bad metadata", data, sizeof(data), 2, ctbytes_4_8, 0, 0, 0);
}

// A FEA1 file without the trailing magic number keeps the whole data block.
static int test_feather_no_metadata(void)
{
    uint8_t data[28] = {0};
    memcpy(data, "FEA1", 4);
    return check_test_file("feather no metadata", data, sizeof(data), 2, ctbytes_4_8, 8, 20, 1);
}

// The file descriptor is closed when fstat fails.
static int test_fstat_error(void)
{
    uint8_t data[28] = {0};
    stub_fstat_fail = 1;
    int errors = check_map_failure("fstat error", data, sizeof(data));
    stub_fstat_fail = 0;
    return errors;
}

// The file descriptor is closed when mmap fails.
static int test_mmap_error(void)
{
    uint8_t data[28] = {0};
    stub_mmap_fail = 1;
    int errors = check_map_failure("mmap error", data, sizeof(data));
    stub_mmap_fail = 0;
    return errors;
}

int main(void)
{
    int errors = 0;

    errors += test_unknown_format();
    errors += test_short_file();
    errors += test_short_file_columns();
    errors += test_columns_out_of_range();
    errors += test_columns_padded();
    errors += test_columns_no_room();
    errors += test_columns_odd_type();
    errors += test_columns_misaligned();
    errors += test_binsrc_truncated();
    errors += test_binsrc_valid();
    errors += test_binsrc_zero_type();
    errors += test_binsrc_bad_index();
    errors += test_binsrc_bad_nrows();
    errors += test_binsrc_no_columns();
    errors += test_binsrc_odd_type();
    errors += test_binsrc_wide_type();
    errors += test_binsrc_misaligned_index();
    errors += test_arrow_bad_metadata();
    errors += test_arrow_bad_dictionary();
    errors += test_arrow_bad_footer();
    errors += test_arrow_no_footer();
    errors += test_feather_bad_metadata();
    errors += test_feather_no_metadata();
    errors += test_fstat_error();
    errors += test_mmap_error();

    return errors;
}
