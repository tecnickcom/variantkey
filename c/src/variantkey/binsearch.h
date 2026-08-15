// BinSearch
//
// binsearch.h
//
// @category   Libraries
// @author     Nicola Asuni <info@tecnick.com>
// @link       https://github.com/tecnickcom/binsearch
// @license    MIT (see LICENSE file)
// @copyright  (c) 2017-2026 Nicola Asuni - Tecnick.com

/**
 * @file binsearch.h
 * @brief Binary search of unsigned integers in memory mapped binary files.
 *
 * Two data layouts are supported:
 *
 *   - Row mode: adjacent constant-length blocks, each holding the searched
 *     value at a fixed offset. In the 8-byte blocks below the first 4 bytes
 *     are a big-endian uint32 and the blocks are sorted by that value.
 *
 *       2f 81 f5 77 1a cc 7b 43
 *       2f 81 f5 78 76 5f 63 b8
 *       2f 81 f5 79 ca a9 a6 52
 *
 *   - Column mode: a contiguous array of unsigned integers of one type, in
 *     host byte order, sorted in ascending order. The "col_" functions take
 *     a typed pointer to that array.
 *
 * The values must be sorted in ascending order in both layouts.
 *
 * The "_be_" functions read big-endian values, the "_le_" functions read
 * little-endian ones.
 *
 * The "_sub_" functions match only the bits from bitstart to bitend of each
 * value, counted from the most significant bit of the type.
 *
 * mmap_binfile() maps a file and reads the header of the BINSRC1, Apache
 * Arrow and Feather formats; for any other content the caller must set the
 * ncols and ctbytes fields before the call.
 *
 * The xxd command-line application converts a binary file to a hexdump and
 * back:
 *
 *   - xxd -p -c8 binaryfile.bin > hexfile.txt
 *   - xxd -r -p hexfile.txt > binaryfile.bin
 */

#ifndef VARIANTKEY_BINSEARCH_H
#define VARIANTKEY_BINSEARCH_H

#include <errno.h>
#include <inttypes.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

// --- BYTE ORDER ---

//!< \cond

// The byte order can be forced by defining BINSEARCH_LITTLE_ENDIAN or
// BINSEARCH_BIG_ENDIAN. WORDS_BIGENDIAN is honoured for autoconf-based builds.

#ifdef WORDS_BIGENDIAN
#undef BINSEARCH_BIG_ENDIAN
#define BINSEARCH_BIG_ENDIAN 1
#endif

#if defined(BINSEARCH_LITTLE_ENDIAN) && defined(BINSEARCH_BIG_ENDIAN)
#error "BINSEARCH_LITTLE_ENDIAN and BINSEARCH_BIG_ENDIAN must not both be defined"
#endif

#if !defined(BINSEARCH_LITTLE_ENDIAN) && !defined(BINSEARCH_BIG_ENDIAN)
#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define BINSEARCH_BIG_ENDIAN 1
#else
// Little-endian is the default when the byte order cannot be detected.
#define BINSEARCH_LITTLE_ENDIAN 1
#endif
#endif

//!< \endcond

/**
 * @brief Reverse the byte order of a 16-bit integer.
 *
 * @param v Value to swap.
 *
 * @return The value with its bytes in the opposite order.
 *
 * @private
 */
static inline uint16_t binsearch_bswap16(uint16_t v)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap16(v);
#elif defined(_MSC_VER)
    return _byteswap_ushort(v);
#else
    return (uint16_t)(((v & (uint16_t)0x00ffU) << 8) | ((v & (uint16_t)0xff00U) >> 8));
#endif
}

/**
 * @brief Reverse the byte order of a 32-bit integer.
 *
 * @param v Value to swap.
 *
 * @return The value with its bytes in the opposite order.
 *
 * @private
 */
static inline uint32_t binsearch_bswap32(uint32_t v)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap32(v);
#elif defined(_MSC_VER)
    return _byteswap_ulong(v);
#else
    return ((v & UINT32_C(0x000000ff)) << 24)
           | ((v & UINT32_C(0x0000ff00)) << 8)
           | ((v & UINT32_C(0x00ff0000)) >> 8)
           | ((v & UINT32_C(0xff000000)) >> 24);
#endif
}

/**
 * @brief Reverse the byte order of a 64-bit integer.
 *
 * @param v Value to swap.
 *
 * @return The value with its bytes in the opposite order.
 *
 * @private
 */
static inline uint64_t binsearch_bswap64(uint64_t v)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap64(v);
#elif defined(_MSC_VER)
    return _byteswap_uint64(v);
#else
    return ((v & UINT64_C(0x00000000000000ff)) << 56)
           | ((v & UINT64_C(0x000000000000ff00)) << 40)
           | ((v & UINT64_C(0x0000000000ff0000)) << 24)
           | ((v & UINT64_C(0x00000000ff000000)) << 8)
           | ((v & UINT64_C(0x000000ff00000000)) >> 8)
           | ((v & UINT64_C(0x0000ff0000000000)) >> 24)
           | ((v & UINT64_C(0x00ff000000000000)) >> 40)
           | ((v & UINT64_C(0xff00000000000000)) >> 56);
#endif
}

#define order_be_uint8_t(x) (x) //!< Return a BE uint8_t in host byte order
#define order_le_uint8_t(x) (x) //!< Return a LE uint8_t in host byte order

#ifdef BINSEARCH_BIG_ENDIAN
#define order_be_uint16_t(x) (x) //!< Return a BE uint16_t in host byte order
#define order_be_uint32_t(x) (x) //!< Return a BE uint32_t in host byte order
#define order_be_uint64_t(x) (x) //!< Return a BE uint64_t in host byte order
#define order_le_uint16_t(x) (binsearch_bswap16(x)) //!< Return a LE uint16_t in host byte order
#define order_le_uint32_t(x) (binsearch_bswap32(x)) //!< Return a LE uint32_t in host byte order
#define order_le_uint64_t(x) (binsearch_bswap64(x)) //!< Return a LE uint64_t in host byte order
#else
#define order_be_uint16_t(x) (binsearch_bswap16(x)) //!< Return a BE uint16_t in host byte order
#define order_be_uint32_t(x) (binsearch_bswap32(x)) //!< Return a BE uint32_t in host byte order
#define order_be_uint64_t(x) (binsearch_bswap64(x)) //!< Return a BE uint64_t in host byte order
#define order_le_uint16_t(x) (x) //!< Return a LE uint16_t in host byte order
#define order_le_uint32_t(x) (x) //!< Return a LE uint32_t in host byte order
#define order_le_uint64_t(x) (x) //!< Return a LE uint64_t in host byte order
#endif

//!< \cond

// O_CLOEXEC keeps the descriptor of a mapped file out of the processes the
// caller execs. It is a POSIX 2008 flag, so it is not visible in a strict ISO C
// build: there it falls back to 0 and the descriptor keeps the default
// behaviour.

#ifdef O_CLOEXEC
#define BINSEARCH_O_CLOEXEC O_CLOEXEC
#else
#define BINSEARCH_O_CLOEXEC 0
#endif

//!< \endcond

#define MAXCOLS 256 //!< Maximum number of indexable columns

/**
 * Return the byte offset of a value inside a memory mapped file.
 *
 * @param blklen    Length of the binary block in bytes.
 * @param blkpos    Byte offset of the value inside a binary block.
 * @param item      Item number.
 *
 * @return Byte offset of the value of the given item.
 */
#define get_address(blklen, blkpos, item) (((blklen) * (item)) + (blkpos))

/**
 * Return the midpoint of a range, rounded down.
 *
 * @param first    First point of the range.
 * @param last     Last point of the range.
 *
 * @return Midpoint of the range.
 */
#define get_middle_point(first, last) ((first) + (((last) - (first)) >> 1))

/**
 * Return a typed pointer to the given byte offset.
 *
 * The offset must be a multiple of the size of T, as required by the
 * column mode functions.
 *
 * @param T        Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t.
 * @param src      Memory mapped file address.
 * @param offset   Byte offset.
 *
 * @return Pointer to T at the given offset.
 */
#define get_src_offset(T, src, offset) ((const T *)((src) + (offset)))

/**
 * Description of a memory mapped file.
 */
typedef struct mmfile_t
{
    uint8_t *src;               //!< Pointer to the memory map, or MAP_FAILED when the file could not be mapped.
    int fd;                     //!< File descriptor, or -1 when the file could not be mapped.
    uint64_t size;              //!< File size in bytes.
    uint64_t doffset;           //!< Byte offset of the first item of the first column.
    uint64_t dlength;           //!< Length in bytes of the data block.
    uint64_t nrows;             //!< Number of rows.
    uint8_t  ncols;             //!< Number of columns. Set by the caller except for the BINSRC1 format.
    uint8_t  ctbytes[MAXCOLS];  //!< Size in bytes of each column type (1, 2, 4 or 8). Set by the caller except for the BINSRC1 format.
    uint64_t index[MAXCOLS];    //!< Byte offset of the first item of each column.
} mmfile_t;

/**
 * Define a function that reads a value of the given type and byte order.
 *
 * @param O Byte order: be or le.
 * @param T Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t.
 */
#define define_bytes_to(O, T) \
/** Read a T value in "O" byte order and return it in host byte order.
The value is read with memcpy because it can be at any byte offset of the
memory mapped file: a typed pointer would be misaligned and would break the
strict aliasing rules. The compiler turns it into a single load.
@param src      Memory mapped file address.
@param i        Byte offset of the value.
@return The value in host byte order.
*/ \
static inline T bytes_##O##_to_##T(const uint8_t *src, uint64_t i) \
{ \
    T v; \
    memcpy(&v, (src + i), sizeof(T)); \
    return order_##O##_##T(v); \
}

define_bytes_to(be, uint8_t)
define_bytes_to(be, uint16_t)
define_bytes_to(be, uint32_t)
define_bytes_to(be, uint64_t)
define_bytes_to(le, uint8_t)
define_bytes_to(le, uint16_t)
define_bytes_to(le, uint32_t)
define_bytes_to(le, uint64_t)

/**
 * Define a function that returns a typed pointer to a byte offset.
 *
 * @param T Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t.
 */
#define define_get_src_offset(T) \
/** Return a pointer to T at the given byte offset.
The offset must be a multiple of the size of T, as required by the column
mode functions.
@param src      Memory mapped file address.
@param offset   Byte offset.
@return Pointer to T at the given offset.
*/ \
static inline const T *get_src_offset_##T(const uint8_t *src, uint64_t offset) \
{ \
    return get_src_offset(T, src, offset); \
}

define_get_src_offset(uint8_t)
define_get_src_offset(uint16_t)
define_get_src_offset(uint32_t)
define_get_src_offset(uint64_t)

#define GET_MIDDLE_BLOCK(O, T) bytes_##O##_to_##T(src, get_address(blklen, blkpos, middle))

#define FIND_START_LOOP_BLOCK(T) \
    uint64_t middle, notfound = *last; \
    T x; \
    while (*first < *last) \
    { \
        middle = get_middle_point(*first, *last); \

// Same as FIND_START_LOOP_BLOCK, plus the first item of the original range:
// the find_last functions inspect the item that precedes the position the loop
// converged to, and must not read below the range the caller asked for.
#define FIND_LAST_START_LOOP_BLOCK(T) \
    const uint64_t firstorig = *first; \
    FIND_START_LOOP_BLOCK(T)

// Opens the block that inspects the candidate item of a find_first search.
// The candidate is only read when "middle" is inside the original range: after
// the loop find_first leaves middle == notfound when the value is above it.
#define FIND_CHECK_BLOCK_START \
    if (middle < notfound) \
    {

// Opens the block that inspects the candidate item of a find_last search: the
// item that precedes the position the loop converged to. It is only read when
// that position is above the first item of the original range, which also
// rules out the wrap-around of the decrement when the range starts at 0.
#define FIND_LAST_CHECK_BLOCK_START \
    if (*first > firstorig) \
    { \
        middle = *first; \
        --middle;

#define FIND_CHECK_BLOCK_END \
        if (x == search) \
        { \
            return middle; \
        } \
    } \
    if (*first > 0) \
    { \
        --(*first); \
    } \
    return notfound;

#define SUB_ITEM_VARS(T) \
    T bitmask = ((T)1 << (bitend - bitstart)); \
    bitmask ^= (bitmask - 1); \
    const uint8_t rshift = (((uint8_t)(sizeof(T) * 8) - 1) - bitend);

#define GET_ITEM_TASK(O, T) \
        x = GET_MIDDLE_BLOCK(O, T);

#define COL_GET_ITEM_TASK \
        x = *(src + middle);

#define GET_SUB_ITEM_TASK(O, T) \
        x = ((GET_MIDDLE_BLOCK(O, T) >> rshift) & bitmask);

#define COL_GET_SUB_ITEM_TASK \
        x = ((*(src + middle) >> rshift) & bitmask);

#define FIND_FIRST_INNER_CHECK \
        if (x < search) { \
            *first = middle; \
            ++(*first); \
        } \
        else \
        { \
            *last = middle; \
        } \
    } \
    middle = *first;

#define FIND_LAST_INNER_CHECK \
        if (x > search) { \
            *last = middle; \
        } \
        else \
        { \
            *first = middle; \
            ++(*first); \
        } \
    }

// The first comparison rejects a position that is already at or past the end of
// the range, including the empty range (last == 0) and the position that would
// make the increment wrap around. The second one is written as
// ((*pos + 1) >= last) rather than (*pos >= (last - 1)) so that an empty range
// does not wrap around either.
#define HAS_NEXT_START_BLOCK \
    if ((*pos >= last) || ((*pos + 1) >= last)) \
    { \
        return 0; \
    } \
    ++(*pos);

#define HAS_PREV_START_BLOCK \
    if (*pos <= first) { \
        return 0; \
    } \
    --(*pos);

#define GET_POS_BLOCK(O, T) bytes_##O##_to_##T(src, get_address(blklen, blkpos, *pos))

#define HAS_END_BLOCK(O, T) \
    return (GET_POS_BLOCK(O, T) == search);

#define COL_HAS_END_BLOCK(T) \
    return (*(src + *pos) == search);

#define HAS_SUB_END_BLOCK(O, T) \
    return (((GET_POS_BLOCK(O, T) >> rshift) & bitmask) == search);

#define COL_HAS_SUB_END_BLOCK(T) \
    return (((*(src + *pos) >> rshift) & bitmask) == search);

/**
 * Define a function that searches for the first occurrence of a value
 * in a memory mapped file of adjacent blocks of sorted binary data.
 *
 * @param O Byte order: be or le.
 * @param T Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t.
 */
#define define_find_first(O, T) \
/** Search for the first occurrence of a T value in a memory mapped file of
adjacent blocks of sorted binary data.
The values must be encoded in "O" byte order and sorted in ascending order.
@param src       Memory mapped file address.
@param blklen    Length of the binary block in bytes.
@param blkpos    Byte offset of the value inside a binary block.
@param first     Pointer to the first item of the search range (min value = 0). Updated on return.
@param last      Pointer to the item past the last one of the search range (max value = nrows). Updated on return.
@param search    Value to search.
@return The item number when the value is found, or the initial value of "last" when it is not.
 */ \
static inline uint64_t find_first_##O##_##T(const uint8_t *src, uint64_t blklen, uint64_t blkpos, uint64_t *first, uint64_t *last, T search) \
{ \
FIND_START_LOOP_BLOCK(T) \
GET_ITEM_TASK(O, T) \
FIND_FIRST_INNER_CHECK \
FIND_CHECK_BLOCK_START \
GET_ITEM_TASK(O, T) \
FIND_CHECK_BLOCK_END \
}

define_find_first(be, uint8_t)
define_find_first(be, uint16_t)
define_find_first(be, uint32_t)
define_find_first(be, uint64_t)
define_find_first(le, uint8_t)
define_find_first(le, uint16_t)
define_find_first(le, uint32_t)
define_find_first(le, uint64_t)

/**
 * Define a function that searches for the first occurrence of a value
 * in a memory mapped file of adjacent blocks of sorted binary data.
 *
 * @param O Byte order: be or le.
 * @param T Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t
 */
#define define_find_first_sub(O, T) \
/** Search for the first occurrence of a T value in a memory mapped file of
adjacent blocks of sorted binary data.
The values must be encoded in "O" byte order and sorted in ascending order.
@param src       Memory mapped file address.
@param blklen    Length of the binary block in bytes.
@param blkpos    Byte offset of the value inside a binary block.
@param bitstart  First bit to match, counted from the most significant bit of T (usually 0).
@param bitend    Last bit to match, counted from the most significant bit of T (7 for uint8_t, 15 for uint16_t, 31 for uint32_t, 63 for uint64_t). Must be at least bitstart and less than the width of T in bits.
@param first     Pointer to the first item of the search range (min value = 0). Updated on return.
@param last      Pointer to the item past the last one of the search range (max value = nrows). Updated on return.
@param search    Value to search.
@return The item number when the value is found, or the initial value of "last" when it is not.
 */ \
static inline uint64_t find_first_sub_##O##_##T(const uint8_t *src, uint64_t blklen, uint64_t blkpos, uint8_t bitstart, uint8_t bitend, uint64_t *first, uint64_t *last, T search) \
{ \
SUB_ITEM_VARS(T) \
FIND_START_LOOP_BLOCK(T) \
GET_SUB_ITEM_TASK(O, T) \
FIND_FIRST_INNER_CHECK \
FIND_CHECK_BLOCK_START \
GET_SUB_ITEM_TASK(O, T) \
FIND_CHECK_BLOCK_END \
}

define_find_first_sub(be, uint8_t)
define_find_first_sub(be, uint16_t)
define_find_first_sub(be, uint32_t)
define_find_first_sub(be, uint64_t)
define_find_first_sub(le, uint8_t)
define_find_first_sub(le, uint16_t)
define_find_first_sub(le, uint32_t)
define_find_first_sub(le, uint64_t)

/**
 * Define a function that searches for the last occurrence of a value
 * in a memory mapped file of adjacent blocks of sorted binary data.
 *
 * @param O Byte order: be or le.
 * @param T Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t
 */
#define define_find_last(O, T) \
/** Search for the last occurrence of a T value in a memory mapped file of
adjacent blocks of sorted binary data.
The values must be encoded in "O" byte order and sorted in ascending order.
@param src       Memory mapped file address.
@param blklen    Length of the binary block in bytes.
@param blkpos    Byte offset of the value inside a binary block.
@param first     Pointer to the first item of the search range (min value = 0). Updated on return.
@param last      Pointer to the item past the last one of the search range (max value = nrows). Updated on return.
@param search    Value to search.
@return The item number when the value is found, or the initial value of "last" when it is not.
*/ \
static inline uint64_t find_last_##O##_##T(const uint8_t *src, uint64_t blklen, uint64_t blkpos, uint64_t *first, uint64_t *last, T search) \
{ \
FIND_LAST_START_LOOP_BLOCK(T) \
GET_ITEM_TASK(O, T) \
FIND_LAST_INNER_CHECK \
FIND_LAST_CHECK_BLOCK_START \
GET_ITEM_TASK(O, T) \
FIND_CHECK_BLOCK_END \
}

define_find_last(be, uint8_t)
define_find_last(be, uint16_t)
define_find_last(be, uint32_t)
define_find_last(be, uint64_t)
define_find_last(le, uint8_t)
define_find_last(le, uint16_t)
define_find_last(le, uint32_t)
define_find_last(le, uint64_t)

/**
 * Define a function that searches for the last occurrence of a value
 * in a memory mapped file of adjacent blocks of sorted binary data.
 *
 * @param O Byte order: be or le.
 * @param T Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t
 */
#define define_find_last_sub(O, T) \
/** Search for the last occurrence of a T value in a memory mapped file of
adjacent blocks of sorted binary data.
The values must be encoded in "O" byte order and sorted in ascending order.
@param src       Memory mapped file address.
@param blklen    Length of the binary block in bytes.
@param blkpos    Byte offset of the value inside a binary block.
@param bitstart  First bit to match, counted from the most significant bit of T (usually 0).
@param bitend    Last bit to match, counted from the most significant bit of T (7 for uint8_t, 15 for uint16_t, 31 for uint32_t, 63 for uint64_t). Must be at least bitstart and less than the width of T in bits.
@param first     Pointer to the first item of the search range (min value = 0). Updated on return.
@param last      Pointer to the item past the last one of the search range (max value = nrows). Updated on return.
@param search    Value to search.
@return The item number when the value is found, or the initial value of "last" when it is not.
*/ \
static inline uint64_t find_last_sub_##O##_##T(const uint8_t *src, uint64_t blklen, uint64_t blkpos, uint8_t bitstart, uint8_t bitend, uint64_t *first, uint64_t *last, T search) \
{ \
SUB_ITEM_VARS(T) \
FIND_LAST_START_LOOP_BLOCK(T) \
GET_SUB_ITEM_TASK(O, T) \
FIND_LAST_INNER_CHECK \
FIND_LAST_CHECK_BLOCK_START \
GET_SUB_ITEM_TASK(O, T) \
FIND_CHECK_BLOCK_END \
}

define_find_last_sub(be, uint8_t)
define_find_last_sub(be, uint16_t)
define_find_last_sub(be, uint32_t)
define_find_last_sub(be, uint64_t)
define_find_last_sub(le, uint8_t)
define_find_last_sub(le, uint16_t)
define_find_last_sub(le, uint32_t)
define_find_last_sub(le, uint64_t)

/**
 * Define a function that checks whether the next item matches the search value.
 *
 * @param O Byte order: be or le.
 * @param T Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t
 */
#define define_has_next(O, T) \
/** Check whether the item after "pos" in a memory mapped file of adjacent
blocks of sorted binary data matches the search value.
The values must be encoded in "O" byte order and sorted in ascending order.
Set "pos" to the item returned by the matching find_first function to walk
forward over the items that satisfy the search.
@param src       Memory mapped file address.
@param blklen    Length of the binary block in bytes.
@param blkpos    Byte offset of the value inside a binary block.
@param pos       Pointer to the current item. Updated to the next item.
@param last      Item past the last one of the search range (max value = nrows).
@param search    Value to search.
@return true when the next item matches the search value, false otherwise.
 */ \
static inline bool has_next_##O##_##T(const uint8_t *src, uint64_t blklen, uint64_t blkpos, uint64_t *pos, uint64_t last, T search) \
{ \
HAS_NEXT_START_BLOCK \
HAS_END_BLOCK(O, T) \
}

define_has_next(be, uint8_t)
define_has_next(be, uint16_t)
define_has_next(be, uint32_t)
define_has_next(be, uint64_t)
define_has_next(le, uint8_t)
define_has_next(le, uint16_t)
define_has_next(le, uint32_t)
define_has_next(le, uint64_t)

/**
 * Define a function that checks whether the next item matches the search value.
 *
 * @param O Byte order: be or le.
 * @param T Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t
 */
#define define_has_next_sub(O, T) \
/** Check whether the item after "pos" in a memory mapped file of adjacent
blocks of sorted binary data matches the search value.
The values must be encoded in "O" byte order and sorted in ascending order.
Set "pos" to the item returned by the matching find_first_sub function to walk
forward over the items that satisfy the search.
@param src       Memory mapped file address.
@param blklen    Length of the binary block in bytes.
@param blkpos    Byte offset of the value inside a binary block.
@param bitstart  First bit to match, counted from the most significant bit of T (usually 0).
@param bitend    Last bit to match, counted from the most significant bit of T (7 for uint8_t, 15 for uint16_t, 31 for uint32_t, 63 for uint64_t). Must be at least bitstart and less than the width of T in bits.
@param pos       Pointer to the current item. Updated to the next item.
@param last      Item past the last one of the search range (max value = nrows).
@param search    Value to search.
@return true when the next item matches the search value, false otherwise.
 */ \
static inline bool has_next_sub_##O##_##T(const uint8_t *src, uint64_t blklen, uint64_t blkpos, uint8_t bitstart, uint8_t bitend, uint64_t *pos, uint64_t last, T search) \
{ \
HAS_NEXT_START_BLOCK \
SUB_ITEM_VARS(T) \
HAS_SUB_END_BLOCK(O, T) \
}

define_has_next_sub(be, uint8_t)
define_has_next_sub(be, uint16_t)
define_has_next_sub(be, uint32_t)
define_has_next_sub(be, uint64_t)
define_has_next_sub(le, uint8_t)
define_has_next_sub(le, uint16_t)
define_has_next_sub(le, uint32_t)
define_has_next_sub(le, uint64_t)

/**
 * Define a function that checks whether the previous item matches the search value.
 *
 * @param O Byte order: be or le.
 * @param T Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t
 */
#define define_has_prev(O, T) \
/** Check whether the item before "pos" in a memory mapped file of adjacent
blocks of sorted binary data matches the search value.
The values must be encoded in "O" byte order and sorted in ascending order.
Set "pos" to the item returned by the matching find_last function to walk
backward over the items that satisfy the search.
@param src       Memory mapped file address.
@param blklen    Length of the binary block in bytes.
@param blkpos    Byte offset of the value inside a binary block.
@param first     First item of the search range (min value = 0).
@param pos       Pointer to the current item. Updated to the previous item.
@param search    Value to search.
@return true when the previous item matches the search value, false otherwise.
 */ \
static inline bool has_prev_##O##_##T(const uint8_t *src, uint64_t blklen, uint64_t blkpos, uint64_t first, uint64_t *pos, T search) \
{ \
HAS_PREV_START_BLOCK \
HAS_END_BLOCK(O, T) \
}

define_has_prev(be, uint8_t)
define_has_prev(be, uint16_t)
define_has_prev(be, uint32_t)
define_has_prev(be, uint64_t)
define_has_prev(le, uint8_t)
define_has_prev(le, uint16_t)
define_has_prev(le, uint32_t)
define_has_prev(le, uint64_t)

/**
 * Define a function that checks whether the previous item matches the search value.
 *
 * @param O Byte order: be or le.
 * @param T Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t
 */
#define define_has_prev_sub(O, T) \
/** Check whether the item before "pos" in a memory mapped file of adjacent
blocks of sorted binary data matches the search value.
The values must be encoded in "O" byte order and sorted in ascending order.
Set "pos" to the item returned by the matching find_last_sub function to walk
backward over the items that satisfy the search.
@param src       Memory mapped file address.
@param blklen    Length of the binary block in bytes.
@param blkpos    Byte offset of the value inside a binary block.
@param bitstart  First bit to match, counted from the most significant bit of T (usually 0).
@param bitend    Last bit to match, counted from the most significant bit of T (7 for uint8_t, 15 for uint16_t, 31 for uint32_t, 63 for uint64_t). Must be at least bitstart and less than the width of T in bits.
@param first     First item of the search range (min value = 0).
@param pos       Pointer to the current item. Updated to the previous item.
@param search    Value to search.
@return true when the previous item matches the search value, false otherwise.
 */ \
static inline bool has_prev_sub_##O##_##T(const uint8_t *src, uint64_t blklen, uint64_t blkpos, uint8_t bitstart, uint8_t bitend, uint64_t first, uint64_t *pos, T search) \
{ \
HAS_PREV_START_BLOCK \
SUB_ITEM_VARS(T) \
HAS_SUB_END_BLOCK(O, T) \
}

define_has_prev_sub(be, uint8_t)
define_has_prev_sub(be, uint16_t)
define_has_prev_sub(be, uint32_t)
define_has_prev_sub(be, uint64_t)
define_has_prev_sub(le, uint8_t)
define_has_prev_sub(le, uint16_t)
define_has_prev_sub(le, uint32_t)
define_has_prev_sub(le, uint64_t)

// --- COLUMN MODE ---

/**
 * Define a function that searches for the first occurrence of a value
 * in a contiguous array of unsigned integers of the same type.
 *
 * @param T Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t.
 */
#define define_col_find_first(T) \
/** Search for the first occurrence of a T value in a contiguous array of
unsigned integers of the same type.
The values must be in host byte order and sorted in ascending order.
@param src       Pointer to the array of values.
@param first     Pointer to the first item of the search range (min value = 0). Updated on return.
@param last      Pointer to the item past the last one of the search range (max value = nrows). Updated on return.
@param search    Value to search.
@return The item number when the value is found, or the initial value of "last" when it is not.
 */ \
static inline uint64_t col_find_first_##T(const T *src, uint64_t *first, uint64_t *last, T search) \
{ \
FIND_START_LOOP_BLOCK(T) \
COL_GET_ITEM_TASK \
FIND_FIRST_INNER_CHECK \
FIND_CHECK_BLOCK_START \
COL_GET_ITEM_TASK \
FIND_CHECK_BLOCK_END \
}

define_col_find_first(uint8_t)
define_col_find_first(uint16_t)
define_col_find_first(uint32_t)
define_col_find_first(uint64_t)

/**
 * Define a function that searches for the first occurrence of a value
 * in a contiguous array of unsigned integers of the same type.
 *
 * @param T Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t
 */
#define define_col_find_first_sub(T) \
/** Search for the first occurrence of a T value in a contiguous array of
unsigned integers of the same type.
The values must be in host byte order and sorted in ascending order.
@param src       Pointer to the array of values.
@param bitstart  First bit to match, counted from the most significant bit of T (usually 0).
@param bitend    Last bit to match, counted from the most significant bit of T (7 for uint8_t, 15 for uint16_t, 31 for uint32_t, 63 for uint64_t). Must be at least bitstart and less than the width of T in bits.
@param first     Pointer to the first item of the search range (min value = 0). Updated on return.
@param last      Pointer to the item past the last one of the search range (max value = nrows). Updated on return.
@param search    Value to search.
@return The item number when the value is found, or the initial value of "last" when it is not.
 */ \
static inline uint64_t col_find_first_sub_##T(const T *src, uint8_t bitstart, uint8_t bitend, uint64_t *first, uint64_t *last, T search) \
{ \
SUB_ITEM_VARS(T) \
FIND_START_LOOP_BLOCK(T) \
COL_GET_SUB_ITEM_TASK \
FIND_FIRST_INNER_CHECK \
FIND_CHECK_BLOCK_START \
COL_GET_SUB_ITEM_TASK \
FIND_CHECK_BLOCK_END \
}

define_col_find_first_sub(uint8_t)
define_col_find_first_sub(uint16_t)
define_col_find_first_sub(uint32_t)
define_col_find_first_sub(uint64_t)

/**
 * Define a function that searches for the last occurrence of a value
 * in a contiguous array of unsigned integers of the same type.
 *
 * @param T Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t
 */
#define define_col_find_last(T) \
/** Search for the last occurrence of a T value in a contiguous array of
unsigned integers of the same type.
The values must be in host byte order and sorted in ascending order.
@param src       Pointer to the array of values.
@param first     Pointer to the first item of the search range (min value = 0). Updated on return.
@param last      Pointer to the item past the last one of the search range (max value = nrows). Updated on return.
@param search    Value to search.
@return The item number when the value is found, or the initial value of "last" when it is not.
*/ \
static inline uint64_t col_find_last_##T(const T *src, uint64_t *first, uint64_t *last, T search) \
{ \
FIND_LAST_START_LOOP_BLOCK(T) \
COL_GET_ITEM_TASK \
FIND_LAST_INNER_CHECK \
FIND_LAST_CHECK_BLOCK_START \
COL_GET_ITEM_TASK \
FIND_CHECK_BLOCK_END \
}

define_col_find_last(uint8_t)
define_col_find_last(uint16_t)
define_col_find_last(uint32_t)
define_col_find_last(uint64_t)

/**
 * Define a function that searches for the last occurrence of a value
 * in a contiguous array of unsigned integers of the same type.
 *
 * @param T Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t
 */
#define define_col_find_last_sub(T) \
/** Search for the last occurrence of a T value in a contiguous array of
unsigned integers of the same type.
The values must be in host byte order and sorted in ascending order.
@param src       Pointer to the array of values.
@param bitstart  First bit to match, counted from the most significant bit of T (usually 0).
@param bitend    Last bit to match, counted from the most significant bit of T (7 for uint8_t, 15 for uint16_t, 31 for uint32_t, 63 for uint64_t). Must be at least bitstart and less than the width of T in bits.
@param first     Pointer to the first item of the search range (min value = 0). Updated on return.
@param last      Pointer to the item past the last one of the search range (max value = nrows). Updated on return.
@param search    Value to search.
@return The item number when the value is found, or the initial value of "last" when it is not.
*/ \
static inline uint64_t col_find_last_sub_##T(const T *src, uint8_t bitstart, uint8_t bitend, uint64_t *first, uint64_t *last, T search) \
{ \
SUB_ITEM_VARS(T) \
FIND_LAST_START_LOOP_BLOCK(T) \
COL_GET_SUB_ITEM_TASK \
FIND_LAST_INNER_CHECK \
FIND_LAST_CHECK_BLOCK_START \
COL_GET_SUB_ITEM_TASK \
FIND_CHECK_BLOCK_END \
}

define_col_find_last_sub(uint8_t)
define_col_find_last_sub(uint16_t)
define_col_find_last_sub(uint32_t)
define_col_find_last_sub(uint64_t)

/**
 * Define a function that checks whether the next item matches the search value.
 *
 * @param T Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t
 */
#define define_col_has_next(T) \
/** Check whether the item after "pos" in a contiguous array of unsigned
integers of the same type matches the search value.
The values must be in host byte order and sorted in ascending order.
Set "pos" to the item returned by the matching col_find_first function to walk
forward over the items that satisfy the search.
@param src       Pointer to the array of values.
@param pos       Pointer to the current item. Updated to the next item.
@param last      Item past the last one of the search range (max value = nrows).
@param search    Value to search.
@return true when the next item matches the search value, false otherwise.
 */ \
static inline bool col_has_next_##T(const T *src, uint64_t *pos, uint64_t last, T search) \
{ \
HAS_NEXT_START_BLOCK \
COL_HAS_END_BLOCK(T) \
}

define_col_has_next(uint8_t)
define_col_has_next(uint16_t)
define_col_has_next(uint32_t)
define_col_has_next(uint64_t)

/**
 * Define a function that checks whether the next item matches the search value.
 *
 * @param T Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t
 */
#define define_col_has_next_sub(T) \
/** Check whether the item after "pos" in a contiguous array of unsigned
integers of the same type matches the search value.
The values must be in host byte order and sorted in ascending order.
Set "pos" to the item returned by the matching col_find_first_sub function to walk
forward over the items that satisfy the search.
@param src       Pointer to the array of values.
@param bitstart  First bit to match, counted from the most significant bit of T (usually 0).
@param bitend    Last bit to match, counted from the most significant bit of T (7 for uint8_t, 15 for uint16_t, 31 for uint32_t, 63 for uint64_t). Must be at least bitstart and less than the width of T in bits.
@param pos       Pointer to the current item. Updated to the next item.
@param last      Item past the last one of the search range (max value = nrows).
@param search    Value to search.
@return true when the next item matches the search value, false otherwise.
 */ \
static inline bool col_has_next_sub_##T(const T *src, uint8_t bitstart, uint8_t bitend, uint64_t *pos, uint64_t last, T search) \
{ \
HAS_NEXT_START_BLOCK \
SUB_ITEM_VARS(T) \
COL_HAS_SUB_END_BLOCK(T) \
}

define_col_has_next_sub(uint8_t)
define_col_has_next_sub(uint16_t)
define_col_has_next_sub(uint32_t)
define_col_has_next_sub(uint64_t)

/**
 * Define a function that checks whether the previous item matches the search value.
 *
 * @param T Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t
 */
#define define_col_has_prev(T) \
/** Check whether the item before "pos" in a contiguous array of unsigned
integers of the same type matches the search value.
The values must be in host byte order and sorted in ascending order.
Set "pos" to the item returned by the matching col_find_last function to walk
backward over the items that satisfy the search.
@param src       Pointer to the array of values.
@param first     First item of the search range (min value = 0).
@param pos       Pointer to the current item. Updated to the previous item.
@param search    Value to search.
@return true when the previous item matches the search value, false otherwise.
 */ \
static inline bool col_has_prev_##T(const T *src, uint64_t first, uint64_t *pos, T search) \
{ \
HAS_PREV_START_BLOCK \
COL_HAS_END_BLOCK(T) \
}

define_col_has_prev(uint8_t)
define_col_has_prev(uint16_t)
define_col_has_prev(uint32_t)
define_col_has_prev(uint64_t)

/**
 * Define a function that checks whether the previous item matches the search value.
 *
 * @param T Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t
 */
#define define_col_has_prev_sub(T) \
/** Check whether the item before "pos" in a contiguous array of unsigned
integers of the same type matches the search value.
The values must be in host byte order and sorted in ascending order.
Set "pos" to the item returned by the matching col_find_last_sub function to walk
backward over the items that satisfy the search.
@param src       Pointer to the array of values.
@param bitstart  First bit to match, counted from the most significant bit of T (usually 0).
@param bitend    Last bit to match, counted from the most significant bit of T (7 for uint8_t, 15 for uint16_t, 31 for uint32_t, 63 for uint64_t). Must be at least bitstart and less than the width of T in bits.
@param first     First item of the search range (min value = 0).
@param pos       Pointer to the current item. Updated to the previous item.
@param search    Value to search.
@return true when the previous item matches the search value, false otherwise.
 */ \
static inline bool col_has_prev_sub_##T(const T *src, uint8_t bitstart, uint8_t bitend, uint64_t first, uint64_t *pos, T search) \
{ \
HAS_PREV_START_BLOCK \
SUB_ITEM_VARS(T) \
COL_HAS_SUB_END_BLOCK(T) \
}

define_col_has_prev_sub(uint8_t)
define_col_has_prev_sub(uint16_t)
define_col_has_prev_sub(uint32_t)
define_col_has_prev_sub(uint64_t)

// --- FILE ---

#define BINSEARCH_MIN_FILE_SIZE 28 //!< Minimum file size required to hold a format header

/**
 * Set the data block of the memory mapped file to empty.
 *
 * The column offsets are cleared as well, so that a caller that ignores the
 * number of rows cannot read from an offset left behind by a rejected header.
 *
 * @param mf Descriptor of the memory mapped file.
 */
static inline void clear_data_block(mmfile_t *mf)
{
    mf->doffset = 0;
    mf->dlength = 0;
    mf->nrows = 0;
    memset(mf->index, 0, sizeof(mf->index));
}

/**
 * Check that every column of the data block can be indexed and is inside the
 * memory mapped file.
 *
 * The type size must be one of the sizes the column mode functions can read
 * (1, 2, 4 or 8 bytes) and the offset must be a multiple of it, because the
 * column mode functions read the values through a typed pointer and a
 * misaligned one is undefined behaviour. Both values can come from a corrupted
 * or hostile file header, so they cannot be trusted.
 *
 * The number of rows is compared against a division instead of multiplying it
 * by the column type size, because the multiplication can overflow when the
 * values come from a corrupted file header.
 *
 * @param mf Descriptor of the memory mapped file.
 *
 * @return true when every column fits in the file, false otherwise.
 */
static inline bool check_col_bounds(const mmfile_t *mf)
{
    uint8_t i = 0;
    for (i = 0; i < mf->ncols; i++)
    {
        if ((mf->ctbytes[i] == 0)
                || (mf->ctbytes[i] > 8)
                || ((mf->ctbytes[i] & (mf->ctbytes[i] - 1)) != 0) // not a power of two
                || ((mf->index[i] & (uint64_t)(mf->ctbytes[i] - 1)) != 0) // misaligned
                || (mf->index[i] > mf->size)
                || (((mf->size - mf->index[i]) / mf->ctbytes[i]) < mf->nrows))
        {
            return false;
        }
    }
    return true;
}

/**
 * Return the number of bytes the given number of rows occupies in a data block
 * that has no column index.
 *
 * Every column but the last is padded to an 8-byte boundary, so the size is not
 * simply the number of rows times the sum of the column type sizes: the padding
 * bytes belong to the block but hold no value.
 *
 * @param mf    Descriptor of the memory mapped file.
 * @param nrows Number of rows.
 *
 * @return Length in bytes of a data block of the given number of rows.
 */
static inline uint64_t col_block_size(const mmfile_t *mf, uint64_t nrows)
{
    uint64_t total = 0;
    uint8_t i = 0;
    for (i = 0; i < mf->ncols; i++)
    {
        const uint64_t b = (nrows * mf->ctbytes[i]);
        total += b;
        if (((uint16_t)i + 1) < (uint16_t)mf->ncols)
        {
            total += ((8 - (b & 7)) & 7); // account for 8-byte padding
        }
    }
    return total;
}

/**
 * Compute the offset of each column in a data block that has no column index.
 *
 * The columns are stored one after the other, each padded to an 8-byte boundary.
 * The data block is cleared when the resulting layout does not fit in the file,
 * as happens when the column types set by the caller do not match the content.
 *
 * @param mf Descriptor of the memory mapped file.
 */
static inline void parse_col_offset(mmfile_t *mf)
{
    uint8_t i = 0;
    uint64_t b = 0;
    mf->index[0] = mf->doffset;
    for (i = 0; i < mf->ncols; i++)
    {
        b += mf->ctbytes[i];
    }
    if (b == 0)
    {
        return;
    }
    // The division ignores the padding between the columns, so it is an upper
    // bound of the number of rows: bring it down until the padded layout fits
    // in the data block. The padding is at most 7 bytes per column and the sum
    // of the column type sizes is at least the number of columns that have one,
    // so the loop takes at most 8 steps.
    mf->nrows = (mf->dlength / b);
    while ((mf->nrows > 0) && (col_block_size(mf, mf->nrows) > mf->dlength))
    {
        --(mf->nrows);
    }
    for (i = 1; i < mf->ncols; i++)
    {
        b = (mf->nrows * mf->ctbytes[(i - 1)]);
        mf->index[i] = mf->index[(i - 1)] + b + ((8 - (b & 7)) & 7); // account for 8-byte padding
    }
    if (!check_col_bounds(mf))
    {
        clear_data_block(mf);
    }
}

/**
 * Read the header of a BINSRC1 file: number of columns, column types,
 * number of rows and column offsets.
 *
 * The data block is cleared when the header declares no column, when it does
 * not fit in the file or when it describes columns that fall outside of it.
 *
 * @param mf Descriptor of the memory mapped file.
 */
static inline void parse_info_binsrc(mmfile_t *mf)
{
    uint64_t ncols = bytes_le_to_uint8_t(mf->src, 8);
    uint64_t offset = 9 + ncols + ((8 - ((ncols + 1) & 7)) & 7); // account for 8-byte padding
    uint64_t idxlen = ((ncols + 1) * 8); // column offsets section
    // A file that declares no column cannot describe any row, so the number of
    // rows that follows the header is meaningless and must not be reported.
    if ((ncols == 0) || ((offset + idxlen) > mf->size))
    {
        clear_data_block(mf);
        return;
    }
    mf->ncols = (uint8_t)ncols;
    mf->nrows = bytes_le_to_uint64_t(mf->src, offset);
    uint64_t i = 0;
    for (i = 0; i < ncols; i++)
    {
        mf->ctbytes[i] = bytes_le_to_uint8_t(mf->src, (9 + i));
        mf->index[i] = bytes_le_to_uint64_t(mf->src, (offset + ((i + 1) * 8)));
    }
    mf->doffset = (offset + idxlen);
    mf->dlength = (mf->size - mf->doffset);
    if (!check_col_bounds(mf))
    {
        clear_data_block(mf);
    }
}

/**
 * Read the header of an Apache Arrow file with a single RecordBatch:
 * skip the metadata, the dictionary and the footer.
 *
 * @param mf Descriptor of the memory mapped file.
 */
static inline void parse_info_arrow(mmfile_t *mf)
{
    uint64_t offset = (uint64_t)bytes_le_to_uint32_t(mf->src, 9) + 13; // skip metadata
    if ((offset + 4) > mf->size)
    {
        clear_data_block(mf);
        return;
    }
    offset += (uint64_t)bytes_le_to_uint32_t(mf->src, offset) + 4; // skip dictionary
    if (offset > mf->size)
    {
        clear_data_block(mf);
        return;
    }
    mf->doffset = offset;
    mf->dlength = (mf->size - offset);
    uint64_t type = bytes_le_to_uint64_t(mf->src, (mf->size - 8));
    if ((type & 0xffffffffffff0000) == 0x31574f5252410000) // magic number "ARROW1" in LE
    {
        uint64_t footer = (uint64_t)bytes_le_to_uint32_t(mf->src, (mf->size - 10)) + 10;
        if (footer > mf->dlength)
        {
            clear_data_block(mf);
            return;
        }
        mf->dlength -= footer; // remove footer
    }
}

/**
 * Read the header of a Feather file: skip the 8-byte header and the metadata.
 *
 * @param mf Descriptor of the memory mapped file.
 */
static inline void parse_info_feather(mmfile_t *mf)
{
    mf->doffset = 8;
    mf->dlength = (mf->size - mf->doffset);
    uint32_t type = bytes_le_to_uint32_t(mf->src, (mf->size - 4));
    if (type == 0x31414546) // magic number "FEA1" in LE
    {
        uint64_t metadata = (uint64_t)bytes_le_to_uint32_t(mf->src, (mf->size - 8)) + 8;
        if (metadata > mf->dlength)
        {
            clear_data_block(mf);
            return;
        }
        mf->dlength -= metadata; // remove metadata
    }
}

/**
 * Memory map the specified file.
 *
 * On failure the descriptor is left with fd = -1, size = 0 and src = MAP_FAILED,
 * and any file descriptor that was opened is closed. errno describes the
 * failure: it is preserved across the close of the cleanup path, which would
 * otherwise overwrite the value set by open, fstat or mmap.
 *
 * The ncols and ctbytes fields must be set by the caller for every format
 * except BINSRC1, which carries them in the file header.
 *
 * The data block is set to empty (nrows = 0) when the header is inconsistent
 * or when the described columns do not fit in the file.
 *
 * @param file Path to the file to map.
 * @param mf   Descriptor of the memory mapped file.
 */
static inline void mmap_binfile(const char *file, mmfile_t *mf)
{
    mf->src = (uint8_t *)MAP_FAILED;
    mf->fd = -1;
    mf->size = 0;
    mf->doffset = 0;
    mf->dlength = 0;
    mf->nrows = 0;
    memset(mf->index, 0, sizeof(mf->index));
    struct stat statbuf;
    int err = 0;
    mf->fd = open(file, O_RDONLY | BINSEARCH_O_CLOEXEC);
    if (mf->fd < 0)
    {
        return;
    }
    if (fstat(mf->fd, &statbuf) < 0)
    {
        err = errno;
        close(mf->fd);
        mf->fd = -1;
        errno = err;
        return;
    }
    if (statbuf.st_size <= 0)
    {
        // An empty file leaves errno untouched, so it is reported explicitly.
        // statbuf is only read here, where fstat is known to have succeeded.
        close(mf->fd);
        mf->fd = -1;
        errno = EINVAL;
        return;
    }
    mf->size = (uint64_t)statbuf.st_size;
#if SIZE_MAX < UINT64_MAX
    // mmap takes a size_t: on a target where it is narrower than the file size
    // the length would be truncated and every later bounds check would compare
    // against a size larger than the mapping.
    if (mf->size > (uint64_t)SIZE_MAX)
    {
        close(mf->fd);
        mf->fd = -1;
        mf->size = 0;
        errno = EFBIG;
        return;
    }
#endif
    mf->src = (uint8_t *)mmap(NULL, (size_t)mf->size, PROT_READ, MAP_PRIVATE, mf->fd, 0);
    if (mf->src == (uint8_t *)MAP_FAILED)
    {
        err = errno;
        close(mf->fd);
        mf->fd = -1;
        mf->size = 0;
        errno = err;
        return;
    }
    mf->dlength = mf->size;
    // A file that is too short to contain a header cannot be sniffed, but its
    // columns are still described by the ncols and ctbytes fields of the caller.
    if (mf->size >= BINSEARCH_MIN_FILE_SIZE)
    {
        uint64_t type = bytes_le_to_uint64_t(mf->src, 0);
        switch (type)
        {
        // Custom binsearch format
        case 0x00314352534e4942: // magic number "BINSRC1" in LE
            parse_info_binsrc(mf);
            return;
        // Basic support for Apache Arrow File format with a single RecordBatch.
        case 0x000031574f525241: // magic number "ARROW1" in LE
            parse_info_arrow(mf);
            break;
        // Basic support for Feather File format.
        case 0x0000000031414546: // magic number "FEA1" in LE
            parse_info_feather(mf);
            break;
        default:
            break;
        }
    }
    parse_col_offset(mf);
}

/**
 * Unmap and close the memory mapped file.
 *
 * The file descriptor is left untouched when the unmapping fails, because a
 * descriptor that does not belong to a valid mapping must not be closed.
 *
 * @param mf Descriptor of the memory mapped file.
 *
 * @return 0 on success, -1 on failure with errno set.
 */
static inline int munmap_binfile(mmfile_t mf)
{
    int err = munmap(mf.src, mf.size);
    if (err != 0)
    {
        return err;
    }
    return close(mf.fd);
}

//!< \cond

// The code generation macros are an implementation detail: they are undefined
// here so that they do not leak into the translation units that include this
// header. The documented ones (MAXCOLS, get_address, get_middle_point,
// get_src_offset and the order_* family) are kept.

#undef define_bytes_to
#undef define_get_src_offset
#undef define_find_first
#undef define_find_first_sub
#undef define_find_last
#undef define_find_last_sub
#undef define_has_next
#undef define_has_next_sub
#undef define_has_prev
#undef define_has_prev_sub
#undef define_col_find_first
#undef define_col_find_first_sub
#undef define_col_find_last
#undef define_col_find_last_sub
#undef define_col_has_next
#undef define_col_has_next_sub
#undef define_col_has_prev
#undef define_col_has_prev_sub

#undef GET_MIDDLE_BLOCK
#undef FIND_START_LOOP_BLOCK
#undef FIND_LAST_START_LOOP_BLOCK
#undef FIND_CHECK_BLOCK_START
#undef FIND_LAST_CHECK_BLOCK_START
#undef FIND_CHECK_BLOCK_END
#undef FIND_FIRST_INNER_CHECK
#undef FIND_LAST_INNER_CHECK
#undef SUB_ITEM_VARS
#undef GET_ITEM_TASK
#undef GET_SUB_ITEM_TASK
#undef COL_GET_ITEM_TASK
#undef COL_GET_SUB_ITEM_TASK
#undef HAS_NEXT_START_BLOCK
#undef HAS_PREV_START_BLOCK
#undef GET_POS_BLOCK
#undef HAS_END_BLOCK
#undef HAS_SUB_END_BLOCK
#undef COL_HAS_END_BLOCK
#undef COL_HAS_SUB_END_BLOCK

//!< \endcond

#endif  // VARIANTKEY_BINSEARCH_H
