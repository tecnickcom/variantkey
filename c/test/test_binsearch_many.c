// BinSearch
//
// test_binsearch_many.c
//
// @category   Test
// @author     Nicola Asuni <info@tecnick.com>
// @link       https://github.com/tecnickcom/binsearch
// @license    MIT (see LICENSE file)
// @copyright  (c) 2017-2026 Nicola Asuni - Tecnick.com

// Tests for the find_range functions, which narrow a range to the items equal
// to a value, and for the find_many functions, which search several values in
// one call.

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
#include <string.h>
#include "../src/variantkey/binsearch.h"

#define ROWS 12   //!< Number of items in the test data
#define BLKLEN 16 //!< Length of each binary block in the test data
#define BLKPOS 3  //!< Byte offset of the value inside a binary block

// The test items, with runs of one, two and three equal values and gaps
// between them. They are held in the most significant byte of the searched
// type, so that the bits from 0 to 7 of every type hold the value below and
// the "_sub_" functions search the same items as the plain ones.
static const uint8_t items[ROWS] = {0x10, 0x20, 0x20, 0x20, 0x30, 0x50, 0x50, 0x70, 0x90, 0xa0, 0xb0, 0xc0};

// The values searched by the tests: below the first item, equal to a run of
// one, two and three items, absent between two items, and above the last one.
#define NSEARCH 12
static const uint8_t searches[NSEARCH] = {0x05, 0x10, 0x20, 0x25, 0x30, 0x50, 0x70, 0x90, 0xa0, 0xb0, 0xc0, 0xd0};

// The half-open range of the items equal to each searched value, over the whole
// of the test data.
static const uint64_t expfirst[NSEARCH] = {0, 0, 1, 4, 4, 5, 7, 8, 9, 10, 11, 12};
static const uint64_t explast[NSEARCH] = {0, 1, 4, 4, 5, 7, 8, 9, 10, 11, 12, 12};

// Column-mode test data, one contiguous array per byte order and type, and the
// same items in row mode.
#define define_data(O, T) \
static T col_##O##_##T[ROWS]; \
static uint8_t row_##O##_##T[ROWS * BLKLEN]; \
/** Writes the test items as "O" values of type T, in both layouts. */ \
static void fill_##O##_##T(void) \
{ \
    const uint8_t shift = (uint8_t)((sizeof(T) - 1) * 8); \
    memset(row_##O##_##T, 0, sizeof(row_##O##_##T)); \
    for (size_t item = 0; item < ROWS; item++) \
    { \
        const T value = order_##O##_##T((T)((T)items[item] << shift)); \
        col_##O##_##T[item] = value; \
        memcpy(row_##O##_##T + (item * BLKLEN) + BLKPOS, &value, sizeof(T)); \
    } \
} \
/** Returns the searched value of type T for the given test value. */ \
static T value_##O##_##T(uint8_t v) \
{ \
    return (T)((T)v << ((sizeof(T) - 1) * 8)); \
}

define_data(be, uint8_t)
define_data(be, uint16_t)
define_data(be, uint32_t)
define_data(be, uint64_t)
define_data(le, uint8_t)
define_data(le, uint16_t)
define_data(le, uint32_t)
define_data(le, uint64_t)

// Checks one result of a find_range function against the expected range.
static int check_range(const char *func, const char *name, size_t k, uint64_t count,
                       uint64_t first, uint64_t last, uint64_t wcount, uint64_t wfirst, uint64_t wlast)
{
    if (count != wcount)
    {
        (void)fprintf_s(stderr, "%s (%s) search %zu count : Expecting %" PRIu64 ", got instead: %" PRIu64 "\n",
                        func, name, k, wcount, count);
        return 1;
    }
    if ((first != wfirst) || (last != wlast))
    {
        (void)fprintf_s(stderr, "%s (%s) search %zu range : Expecting [%" PRIu64 ", %" PRIu64 "), got instead: [%" PRIu64 ", %" PRIu64 ")\n",
                        func, name, k, wfirst, wlast, first, last);
        return 1;
    }
    return 0;
}

// The find_range and find_many tests of one byte order and type.
//
// Each find_range result is checked against the expected range over the whole
// of the test data, and then, over every sub-range of it, against the pair of
// find_first and find_last it replaces. Each find_many result is checked
// against the find_first it replaces, over every sub-range and every number of
// searched values from none to more than two batches.
#define define_tests(O, T) \
static int test_range_##O##_##T(void) \
{ \
    int errors = 0; \
    size_t k = 0; \
    const uint8_t topbit = (uint8_t)((sizeof(T) * 8) - 1); \
    fill_##O##_##T(); \
    for (k = 0; k < NSEARCH; k++) \
    { \
        const T s = value_##O##_##T(searches[k]); \
        uint64_t first = 0; \
        uint64_t last = ROWS; \
        uint64_t n = col_find_range_##O##_##T(col_##O##_##T, &first, &last, s); \
        errors += check_range("col_find_range_" #O "_" #T, "col", k, n, first, last, (explast[k] - expfirst[k]), expfirst[k], explast[k]); \
        first = 0; \
        last = ROWS; \
        n = find_range_##O##_##T(row_##O##_##T, BLKLEN, BLKPOS, &first, &last, s); \
        errors += check_range("find_range_" #O "_" #T, "row", k, n, first, last, (explast[k] - expfirst[k]), expfirst[k], explast[k]); \
        /* The whole value and its most significant byte select the same items. */ \
        first = 0; \
        last = ROWS; \
        n = col_find_range_sub_##O##_##T(col_##O##_##T, 0, topbit, &first, &last, s); \
        errors += check_range("col_find_range_sub_" #O "_" #T, "col whole", k, n, first, last, (explast[k] - expfirst[k]), expfirst[k], explast[k]); \
        first = 0; \
        last = ROWS; \
        n = find_range_sub_##O##_##T(row_##O##_##T, BLKLEN, BLKPOS, 0, topbit, &first, &last, s); \
        errors += check_range("find_range_sub_" #O "_" #T, "row whole", k, n, first, last, (explast[k] - expfirst[k]), expfirst[k], explast[k]); \
        first = 0; \
        last = ROWS; \
        n = col_find_range_sub_##O##_##T(col_##O##_##T, 0, 7, &first, &last, (T)searches[k]); \
        errors += check_range("col_find_range_sub_" #O "_" #T, "col byte", k, n, first, last, (explast[k] - expfirst[k]), expfirst[k], explast[k]); \
        first = 0; \
        last = ROWS; \
        n = find_range_sub_##O##_##T(row_##O##_##T, BLKLEN, BLKPOS, 0, 7, &first, &last, (T)searches[k]); \
        errors += check_range("find_range_sub_" #O "_" #T, "row byte", k, n, first, last, (explast[k] - expfirst[k]), expfirst[k], explast[k]); \
    } \
    /* Every sub-range, including the empty and the inverted ones, against the */ \
    /* pair of searches that find_range replaces. */ \
    for (uint64_t f = 0; f <= ROWS; f++) \
    { \
        for (uint64_t l = 0; l <= ROWS; l++) \
        { \
            for (k = 0; k < NSEARCH; k++) \
            { \
                const T s = value_##O##_##T(searches[k]); \
                uint64_t wf = f; \
                uint64_t wl = l; \
                const uint64_t hit = col_find_first_##O##_##T(col_##O##_##T, &wf, &wl, s); \
                uint64_t wcount = 0; \
                uint64_t wfirst = wl; \
                uint64_t wlast = wl; \
                if (hit != l) \
                { \
                    uint64_t lf = f; \
                    uint64_t ll = l; \
                    (void)col_find_last_##O##_##T(col_##O##_##T, &lf, &ll, s); \
                    wlast = ll; \
                    wcount = (wlast - wfirst); \
                } \
                uint64_t first = f; \
                uint64_t last = l; \
                uint64_t n = col_find_range_##O##_##T(col_##O##_##T, &first, &last, s); \
                if (n != wcount) \
                { \
                    (void)fprintf_s(stderr, "col_find_range_" #O "_" #T " [%" PRIu64 ", %" PRIu64 ") search %zu : Expecting %" PRIu64 " items, got instead: %" PRIu64 "\n", f, l, k, wcount, n); \
                    errors++; \
                } \
                else if ((n > 0) && ((first != wfirst) || (last != wlast))) \
                { \
                    (void)fprintf_s(stderr, "col_find_range_" #O "_" #T " [%" PRIu64 ", %" PRIu64 ") search %zu : Expecting [%" PRIu64 ", %" PRIu64 "), got instead: [%" PRIu64 ", %" PRIu64 ")\n", f, l, k, wfirst, wlast, first, last); \
                    errors++; \
                } \
                first = f; \
                last = l; \
                n = find_range_##O##_##T(row_##O##_##T, BLKLEN, BLKPOS, &first, &last, s); \
                if (n != wcount) \
                { \
                    (void)fprintf_s(stderr, "find_range_" #O "_" #T " [%" PRIu64 ", %" PRIu64 ") search %zu : Expecting %" PRIu64 " items, got instead: %" PRIu64 "\n", f, l, k, wcount, n); \
                    errors++; \
                } \
                /* The whole value selects the same items as the plain search. */ \
                first = f; \
                last = l; \
                n = col_find_range_sub_##O##_##T(col_##O##_##T, 0, topbit, &first, &last, s); \
                if ((n != wcount) || ((n > 0) && ((first != wfirst) || (last != wlast)))) \
                { \
                    (void)fprintf_s(stderr, "col_find_range_sub_" #O "_" #T " [%" PRIu64 ", %" PRIu64 ") search %zu : Expecting %" PRIu64 " items in [%" PRIu64 ", %" PRIu64 "), got instead: %" PRIu64 " in [%" PRIu64 ", %" PRIu64 ")\n", f, l, k, wcount, wfirst, wlast, n, first, last); \
                    errors++; \
                } \
                first = f; \
                last = l; \
                n = find_range_sub_##O##_##T(row_##O##_##T, BLKLEN, BLKPOS, 0, topbit, &first, &last, s); \
                if ((n != wcount) || ((n > 0) && ((first != wfirst) || (last != wlast)))) \
                { \
                    (void)fprintf_s(stderr, "find_range_sub_" #O "_" #T " [%" PRIu64 ", %" PRIu64 ") search %zu : Expecting %" PRIu64 " items in [%" PRIu64 ", %" PRIu64 "), got instead: %" PRIu64 " in [%" PRIu64 ", %" PRIu64 ")\n", f, l, k, wcount, wfirst, wlast, n, first, last); \
                    errors++; \
                } \
            } \
        } \
    } \
    return errors; \
} \
static int test_many_##O##_##T(void) \
{ \
    int errors = 0; \
    const uint8_t topbit = (uint8_t)((sizeof(T) * 8) - 1); \
    T search[NSEARCH * 2]; \
    T bytes[NSEARCH * 2]; \
    uint64_t pos[NSEARCH * 2]; \
    size_t k = 0; \
    fill_##O##_##T(); \
    for (k = 0; k < (NSEARCH * 2); k++) \
    { \
        search[k] = value_##O##_##T(searches[k % NSEARCH]); \
        bytes[k] = (T)searches[k % NSEARCH]; \
    } \
    /* Every number of searched values from none to more than two batches, over */ \
    /* every sub-range, against the find_first each of them replaces. */ \
    static const binsearch_prefetch_t modes[3] = {BINSEARCH_PREFETCH_AUTO, BINSEARCH_PREFETCH_ALWAYS, BINSEARCH_PREFETCH_NEVER}; \
    size_t mi = 0; \
    for (uint64_t last = 0; last <= ROWS; last++) \
    { \
        for (uint64_t count = 0; count <= (NSEARCH * 2); count++) \
        { \
            for (mi = 0; mi < 3; mi++) \
            { \
            const binsearch_prefetch_t pf = modes[mi]; \
            memset(pos, 0xff, sizeof(pos)); \
            col_find_many_##O##_##T(col_##O##_##T, 0, last, search, pos, count, pf); \
            for (k = 0; k < count; k++) \
            { \
                uint64_t f = 0; \
                uint64_t l = last; \
                const uint64_t want = col_find_first_##O##_##T(col_##O##_##T, &f, &l, search[k]); \
                if (pos[k] != want) \
                { \
                    (void)fprintf_s(stderr, "col_find_many_" #O "_" #T " last %" PRIu64 " count %" PRIu64 " item %zu : Expecting %" PRIu64 ", got instead: %" PRIu64 "\n", last, count, k, want, pos[k]); \
                    errors++; \
                } \
            } \
            memset(pos, 0xff, sizeof(pos)); \
            find_many_##O##_##T(row_##O##_##T, BLKLEN, BLKPOS, 0, last, search, pos, count, pf); \
            for (k = 0; k < count; k++) \
            { \
                uint64_t f = 0; \
                uint64_t l = last; \
                const uint64_t want = find_first_##O##_##T(row_##O##_##T, BLKLEN, BLKPOS, &f, &l, search[k]); \
                if (pos[k] != want) \
                { \
                    (void)fprintf_s(stderr, "find_many_" #O "_" #T " last %" PRIu64 " count %" PRIu64 " item %zu : Expecting %" PRIu64 ", got instead: %" PRIu64 "\n", last, count, k, want, pos[k]); \
                    errors++; \
                } \
            } \
            memset(pos, 0xff, sizeof(pos)); \
            col_find_many_sub_##O##_##T(col_##O##_##T, 0, topbit, 0, last, search, pos, count, pf); \
            for (k = 0; k < count; k++) \
            { \
                uint64_t f = 0; \
                uint64_t l = last; \
                const uint64_t want = col_find_first_sub_##O##_##T(col_##O##_##T, 0, topbit, &f, &l, search[k]); \
                if (pos[k] != want) \
                { \
                    (void)fprintf_s(stderr, "col_find_many_sub_" #O "_" #T " last %" PRIu64 " count %" PRIu64 " item %zu : Expecting %" PRIu64 ", got instead: %" PRIu64 "\n", last, count, k, want, pos[k]); \
                    errors++; \
                } \
            } \
            memset(pos, 0xff, sizeof(pos)); \
            find_many_sub_##O##_##T(row_##O##_##T, BLKLEN, BLKPOS, 0, 7, 0, last, bytes, pos, count, pf); \
            for (k = 0; k < count; k++) \
            { \
                uint64_t f = 0; \
                uint64_t l = last; \
                const uint64_t want = find_first_sub_##O##_##T(row_##O##_##T, BLKLEN, BLKPOS, 0, 7, &f, &l, bytes[k]); \
                if (pos[k] != want) \
                { \
                    (void)fprintf_s(stderr, "find_many_sub_" #O "_" #T " last %" PRIu64 " count %" PRIu64 " item %zu : Expecting %" PRIu64 ", got instead: %" PRIu64 "\n", last, count, k, want, pos[k]); \
                    errors++; \
                } \
            } \
            } \
        } \
    } \
    return errors; \
}

define_tests(be, uint8_t)
define_tests(be, uint16_t)
define_tests(be, uint32_t)
define_tests(be, uint64_t)
define_tests(le, uint8_t)
define_tests(le, uint16_t)
define_tests(le, uint32_t)
define_tests(le, uint64_t)

int main(void)
{
    int errors = 0;

    errors += test_range_be_uint8_t();
    errors += test_range_be_uint16_t();
    errors += test_range_be_uint32_t();
    errors += test_range_be_uint64_t();
    errors += test_range_le_uint8_t();
    errors += test_range_le_uint16_t();
    errors += test_range_le_uint32_t();
    errors += test_range_le_uint64_t();

    errors += test_many_be_uint8_t();
    errors += test_many_be_uint16_t();
    errors += test_many_be_uint32_t();
    errors += test_many_be_uint64_t();
    errors += test_many_le_uint8_t();
    errors += test_many_le_uint16_t();
    errors += test_many_le_uint32_t();
    errors += test_many_le_uint64_t();

    return (errors > 0) ? 1 : 0;
}
