// BinSearch
//
// test_binsearch_range.c
//
// @category   Test
// @author     Nicola Asuni <info@tecnick.com>
// @link       https://github.com/tecnickcom/binsearch
// @license    MIT (see LICENSE file)
// @copyright  (c) 2017-2026 Nicola Asuni - Tecnick.com

// Tests for searches of values that fall outside the range of the source data,
// and for the iteration helpers called from a position outside the range.

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

#define ROWS 3   //!< Number of items in the test data
#define BLKLEN 8 //!< Length of each binary block in the test data

#define VAL_LOW 0x05  //!< Value below the first item
#define VAL_MID 0x20  //!< Value equal to the second item
#define VAL_HIGH 0x40 //!< Value above the last item

// The three items of the row-mode test data.
static const uint64_t items[ROWS] = {0x10, 0x20, 0x30};

// Column-mode test data, one contiguous array per byte order and type.
#define define_col_data(O, T) \
static T col_##O##_##T[ROWS]; \
/** Writes the test items into the array as "O" values of type T. */ \
static void fill_col_##O##_##T(void) \
{ \
    for (size_t item = 0; item < ROWS; item++) \
    { \
        col_##O##_##T[item] = order_##O##_##T((T)items[item]); \
    } \
}

define_col_data(be, uint8_t)
define_col_data(be, uint16_t)
define_col_data(be, uint32_t)
define_col_data(be, uint64_t)
define_col_data(le, uint8_t)
define_col_data(le, uint16_t)
define_col_data(le, uint32_t)
define_col_data(le, uint64_t)

static uint8_t buf[ROWS * BLKLEN];

// Writes the test items into buf as big-endian values of the given width.
static void fill_be(size_t width)
{
    memset(buf, 0, sizeof(buf));
    for (size_t item = 0; item < ROWS; item++)
    {
        for (size_t i = 0; i < width; i++)
        {
            buf[(item * BLKLEN) + i] = (uint8_t)(items[item] >> ((width - 1 - i) * 8));
        }
    }
}

// Writes the test items into buf as little-endian values of the given width.
static void fill_le(size_t width)
{
    memset(buf, 0, sizeof(buf));
    for (size_t item = 0; item < ROWS; item++)
    {
        for (size_t i = 0; i < width; i++)
        {
            buf[(item * BLKLEN) + i] = (uint8_t)(items[item] >> (i * 8));
        }
    }
}

static int errors = 0;

// Reports a mismatch between the returned item number and the expected one.
static void check(const char *name, uint64_t got, uint64_t exp)
{
    if (got != exp)
    {
        (void)fprintf_s(stderr, "%s : Expecting %" PRIu64 ", got instead: %" PRIu64 "\n", name, exp, got);
        errors++;
    }
}

// The searched value sits at item 1, so a search restricted to the items from
// SUBFIRST on must not report it: the item that precedes the range is only one
// position away and find_last inspects exactly that position.
#define SUBFIRST 2

// Runs the found, below-range, above-range and below-sub-range cases of a
// row-mode function, then the ranges of one and of no item at all: the search
// loop halves the range until one item is left, so those are the cases that
// take its last step and the one that skips it.
#define CHECK_ROW(FUNC, T, ...) \
    { \
        uint64_t first = 0; \
        uint64_t last = ROWS; \
        check(#FUNC " found", FUNC(buf, BLKLEN, 0, __VA_ARGS__ &first, &last, (T)VAL_MID), 1); \
        first = 0; \
        last = ROWS; \
        check(#FUNC " below", FUNC(buf, BLKLEN, 0, __VA_ARGS__ &first, &last, (T)VAL_LOW), ROWS); \
        first = 0; \
        last = ROWS; \
        check(#FUNC " above", FUNC(buf, BLKLEN, 0, __VA_ARGS__ &first, &last, (T)VAL_HIGH), ROWS); \
        first = SUBFIRST; \
        last = ROWS; \
        check(#FUNC " below sub-range", FUNC(buf, BLKLEN, 0, __VA_ARGS__ &first, &last, (T)VAL_MID), ROWS); \
        first = 1; \
        last = 1; \
        check(#FUNC " empty range", FUNC(buf, BLKLEN, 0, __VA_ARGS__ &first, &last, (T)VAL_MID), 1); \
        first = 1; \
        last = 2; \
        check(#FUNC " one item found", FUNC(buf, BLKLEN, 0, __VA_ARGS__ &first, &last, (T)VAL_MID), 1); \
        first = 0; \
        last = 1; \
        check(#FUNC " one item above", FUNC(buf, BLKLEN, 0, __VA_ARGS__ &first, &last, (T)VAL_HIGH), 1); \
        first = 0; \
        last = 1; \
        check(#FUNC " one item below", FUNC(buf, BLKLEN, 0, __VA_ARGS__ &first, &last, (T)VAL_LOW), 1); \
    }

// Same as CHECK_ROW for a column-mode function.
#define CHECK_COL(FUNC, O, T, ...) \
    { \
        uint64_t first = 0; \
        uint64_t last = ROWS; \
        check(#FUNC " found", FUNC(col_##O##_##T, __VA_ARGS__ &first, &last, (T)VAL_MID), 1); \
        first = 0; \
        last = ROWS; \
        check(#FUNC " below", FUNC(col_##O##_##T, __VA_ARGS__ &first, &last, (T)VAL_LOW), ROWS); \
        first = 0; \
        last = ROWS; \
        check(#FUNC " above", FUNC(col_##O##_##T, __VA_ARGS__ &first, &last, (T)VAL_HIGH), ROWS); \
        first = SUBFIRST; \
        last = ROWS; \
        check(#FUNC " below sub-range", FUNC(col_##O##_##T, __VA_ARGS__ &first, &last, (T)VAL_MID), ROWS); \
        first = 1; \
        last = 1; \
        check(#FUNC " empty range", FUNC(col_##O##_##T, __VA_ARGS__ &first, &last, (T)VAL_MID), 1); \
        first = 1; \
        last = 2; \
        check(#FUNC " one item found", FUNC(col_##O##_##T, __VA_ARGS__ &first, &last, (T)VAL_MID), 1); \
        first = 0; \
        last = 1; \
        check(#FUNC " one item above", FUNC(col_##O##_##T, __VA_ARGS__ &first, &last, (T)VAL_HIGH), 1); \
        first = 0; \
        last = 1; \
        check(#FUNC " one item below", FUNC(col_##O##_##T, __VA_ARGS__ &first, &last, (T)VAL_LOW), 1); \
    }

// Checks that a row-mode has_next function reports no next item when the
// position is already at the end of the range, and that a position that would
// make the increment wrap around is rejected instead of restarting from item 0.
#define CHECK_HAS_NEXT_ROW(FUNC, T, ...) \
    { \
        uint64_t pos = ROWS; \
        check(#FUNC " at end", (uint64_t)FUNC(buf, BLKLEN, 0, __VA_ARGS__ &pos, ROWS, (T)VAL_MID), 0); \
        check(#FUNC " at end pos", pos, ROWS); \
        pos = UINT64_MAX; \
        check(#FUNC " wrap", (uint64_t)FUNC(buf, BLKLEN, 0, __VA_ARGS__ &pos, ROWS, (T)VAL_MID), 0); \
        check(#FUNC " wrap pos", pos, UINT64_MAX); \
    }

// Same as CHECK_HAS_NEXT_ROW for a column-mode has_next function.
#define CHECK_HAS_NEXT_COL(FUNC, O, T, ...) \
    { \
        uint64_t pos = ROWS; \
        check(#FUNC " at end", (uint64_t)FUNC(col_##O##_##T, __VA_ARGS__ &pos, ROWS, (T)VAL_MID), 0); \
        check(#FUNC " at end pos", pos, ROWS); \
        pos = UINT64_MAX; \
        check(#FUNC " wrap", (uint64_t)FUNC(col_##O##_##T, __VA_ARGS__ &pos, ROWS, (T)VAL_MID), 0); \
        check(#FUNC " wrap pos", pos, UINT64_MAX); \
    }

// Runs every row-mode search for one byte order and type.
#define CHECK_ROW_TYPE(O, T, WIDTH, LASTBIT) \
    { \
        fill_##O((WIDTH)); \
        CHECK_ROW(find_first_##O##_##T, T,) \
        CHECK_ROW(find_last_##O##_##T, T,) \
        CHECK_ROW(find_first_sub_##O##_##T, T, 0, (LASTBIT),) \
        CHECK_ROW(find_last_sub_##O##_##T, T, 0, (LASTBIT),) \
        CHECK_HAS_NEXT_ROW(has_next_##O##_##T, T,) \
        CHECK_HAS_NEXT_ROW(has_next_sub_##O##_##T, T, 0, (LASTBIT),) \
    }

// Runs every column-mode search for one byte order and type.
#define CHECK_COL_TYPE(O, T, LASTBIT) \
    { \
        fill_col_##O##_##T(); \
        CHECK_COL(col_find_first_##O##_##T, O, T,) \
        CHECK_COL(col_find_last_##O##_##T, O, T,) \
        CHECK_COL(col_find_first_sub_##O##_##T, O, T, 0, (LASTBIT),) \
        CHECK_COL(col_find_last_sub_##O##_##T, O, T, 0, (LASTBIT),) \
        CHECK_HAS_NEXT_COL(col_has_next_##O##_##T, O, T,) \
        CHECK_HAS_NEXT_COL(col_has_next_sub_##O##_##T, O, T, 0, (LASTBIT),) \
    }

int main(void)
{
    CHECK_ROW_TYPE(be, uint8_t, 1, 7)
    CHECK_ROW_TYPE(be, uint16_t, 2, 15)
    CHECK_ROW_TYPE(be, uint32_t, 4, 31)
    CHECK_ROW_TYPE(be, uint64_t, 8, 63)
    CHECK_ROW_TYPE(le, uint8_t, 1, 7)
    CHECK_ROW_TYPE(le, uint16_t, 2, 15)
    CHECK_ROW_TYPE(le, uint32_t, 4, 31)
    CHECK_ROW_TYPE(le, uint64_t, 8, 63)

    CHECK_COL_TYPE(be, uint8_t, 7)
    CHECK_COL_TYPE(be, uint16_t, 15)
    CHECK_COL_TYPE(be, uint32_t, 31)
    CHECK_COL_TYPE(be, uint64_t, 63)
    CHECK_COL_TYPE(le, uint8_t, 7)
    CHECK_COL_TYPE(le, uint16_t, 15)
    CHECK_COL_TYPE(le, uint32_t, 31)
    CHECK_COL_TYPE(le, uint64_t, 63)

    return errors;
}
