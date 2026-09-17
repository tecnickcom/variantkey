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
 *   - Column mode: a contiguous array of unsigned integers of one type.
 *     The "col_" functions take a typed pointer to that array.
 *
 * The values must be sorted in ascending order in both layouts.
 *
 * The "_be_" functions read big-endian values, the "_le_" functions read
 * little-endian ones, in both layouts.
 *
 * The "_sub_" functions match only the bits from bitstart to bitend of each
 * value, counted from the most significant bit of the type.
 *
 * The find functions narrow the first and last arguments to a bound of the
 * searched value inside the range they were given:
 *
 *   - find_first converges to the lower bound, the first item that is not less
 *     than the searched value;
 *   - find_last converges to the upper bound, the first item that is greater
 *     than the searched value.
 *
 * On return "last" is that bound and "first" is the same position, or one below
 * it when the value is absent. So find_first writes the item it found while
 * find_last writes the position after it, and neither describes a range to
 * iterate: the has_next and has_prev functions take the bounds of the original
 * range.
 *
 * The two bounds delimit the items equal to the searched value: the "last" of
 * find_first is where they start and the "last" of find_last is where they end,
 * as a half-open range. The find_range functions compute both in one descent,
 * which costs about half of the two separate searches on data larger than the
 * cache, and return the number of items in it.
 *
 * The find_many functions search several values in one call. They advance the
 * searches in lockstep, so the cache misses of the whole batch are outstanding
 * together instead of one at a time, which is about three to four times as fast
 * per value on data larger than the CPU cache and still held in RAM. On a file
 * too large for RAM the misses become page faults, which the kernel serves one
 * at a time; the prefetch argument selects whether the pages of a batch are
 * asked for before they are read, which puts those reads in the device queue
 * together.
 *
 * The search functions do not check their arguments. The
 * binsearch_check_row_range, binsearch_check_col_range,
 * binsearch_check_col_offset and binsearch_check_bits functions are the checks
 * a caller applies first when it takes them from a file header or from its own
 * input, and the ones the Go and Python bindings apply on every search.
 *
 * mmap_binfile() maps a file and reads the header of the BINSRC1, Apache
 * Arrow and Feather formats; for any other content the caller must set the
 * ncols and ctbytes fields before the call. It reports the byte order of the
 * values in the border field, which the caller uses to pick between the
 * "_be_" and the "_le_" functions. No supported format declares a byte order
 * this reads, so the field is always BINSEARCH_ORDER_LE and a caller whose
 * content is big-endian overwrites it.
 *
 * A mapped file may be searched from several threads at once, as the searches
 * only read it. munmap_binfile() must not run at the same time as a search.
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
#include <time.h>
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
// caller execs. O_NONBLOCK keeps the open from waiting on a path that names a
// FIFO, which waits for a writer, or a device that waits for a carrier; it does
// not affect the reads of a regular file. Both are POSIX flags, so neither is
// guaranteed to be visible in a strict ISO C build, and each one is dropped
// from the set when it is not.

#if defined(O_CLOEXEC) && defined(O_NONBLOCK)
#define BINSEARCH_OPEN_FLAGS (O_RDONLY | O_CLOEXEC | O_NONBLOCK)
#elif defined(O_CLOEXEC)
#define BINSEARCH_OPEN_FLAGS (O_RDONLY | O_CLOEXEC)
#elif defined(O_NONBLOCK)
#define BINSEARCH_OPEN_FLAGS (O_RDONLY | O_NONBLOCK)
#else
#define BINSEARCH_OPEN_FLAGS (O_RDONLY)
#endif

// S_ISREG is a POSIX macro. When it is not visible the file type cannot be
// checked, so every file is accepted.

#ifndef S_ISREG
#define S_ISREG(m) (1)
#endif

// MADV_RANDOM tells the kernel that the mapping is read in a scattered order,
// which is what a binary search does. It is not ISO C, so binsearch_advise_random
// reports that it did nothing when the platform does not provide it.

#if defined(POSIX_MADV_RANDOM)
#define BINSEARCH_HAVE_ADVISE 1
#define BINSEARCH_ADVISE(addr, len) posix_madvise((addr), (len), POSIX_MADV_RANDOM)
#elif defined(MADV_RANDOM)
#define BINSEARCH_HAVE_ADVISE 1
#define BINSEARCH_ADVISE(addr, len) madvise((addr), (len), MADV_RANDOM)
#else
#define BINSEARCH_HAVE_ADVISE 0
#define BINSEARCH_ADVISE(addr, len) (-1)
#endif

// MADV_WILLNEED asks for a page to be read before it is touched. The find_many
// functions use it to put the reads of a whole batch in the device queue at
// once; see the comment on FIND_MANY_VARS. It is not ISO C, so the prefetch is
// compiled out when the platform does not provide it.

#if defined(POSIX_MADV_WILLNEED)
#define BINSEARCH_HAVE_WILLNEED 1
#define BINSEARCH_WILLNEED(addr, len) posix_madvise((addr), (len), POSIX_MADV_WILLNEED)
#elif defined(MADV_WILLNEED)
#define BINSEARCH_HAVE_WILLNEED 1
#define BINSEARCH_WILLNEED(addr, len) madvise((addr), (len), MADV_WILLNEED)
#else
#define BINSEARCH_HAVE_WILLNEED 0
#define BINSEARCH_WILLNEED(addr, len) (-1)
#endif

//!< \endcond

#define MAXCOLS 255 //!< Maximum number of indexable columns, as limited by the width of the ncols field

#ifndef BINSEARCH_BATCH
#define BINSEARCH_BATCH 8 //!< Number of searches the find_many functions keep in flight at once
#endif

#ifndef BINSEARCH_PROBE_EVERY
#define BINSEARCH_PROBE_EVERY 64 //!< Number of batches between two residency probes. Must be a power of two.
#endif

#ifndef BINSEARCH_SLOW_NS
#define BINSEARCH_SLOW_NS 5000 //!< Nanoseconds per search above which a probed batch counts as reading from a device
#endif

/**
 * Whether the find_many functions ask for the pages of a batch before reading
 * them.
 *
 * The request only pays off when the pages are not resident: on data already in
 * memory it is a system call per value per step that buys nothing, and costs
 * more than the search.
 */
typedef enum binsearch_prefetch_t
{
    BINSEARCH_PREFETCH_AUTO = 0,   //!< Ask when a timed batch is slow enough to be reading from a device.
    BINSEARCH_PREFETCH_ALWAYS = 1, //!< Always ask.
    BINSEARCH_PREFETCH_NEVER = 2,  //!< Never ask.
} binsearch_prefetch_t;

//!< \cond

#if BINSEARCH_HAVE_WILLNEED

// Reading the clock is only needed by BINSEARCH_PREFETCH_AUTO, and only once
// per probed batch.

// CLOCK_MONOTONIC is POSIX and is not declared in a strict ISO C build, which
// falls back to the C11 timespec_get. That one follows the wall clock, so it can
// step; the worst a step does here is decide one probe wrongly, and the next
// probe corrects it.

static inline uint64_t binsearch_now_ns(void)
{
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = 0;
#if defined(CLOCK_MONOTONIC)
    (void)clock_gettime(CLOCK_MONOTONIC, &t);
#else
    (void)timespec_get(&t, TIME_UTC);
#endif
    return (((uint64_t)t.tv_sec * 1000000000) + (uint64_t)t.tv_nsec);
}

// madvise takes a page aligned address, so the page size is needed to round one
// down. It is read once per find_many call that prefetches, never on the
// resident path.

// _SC_PAGESIZE is one of the values POSIX requires sysconf to answer, so the
// call does not fail. A wrong answer would only make the request cover the
// wrong range, which the kernel rejects and the caller ignores.

static inline uint64_t binsearch_page_size(void)
{
#ifdef _SC_PAGESIZE
    return (uint64_t)sysconf(_SC_PAGESIZE);
#else
    return 4096;
#endif
}

#endif // BINSEARCH_HAVE_WILLNEED

//!< \endcond

/**
 * Check that every item of a row mode search is inside a block of memory.
 *
 * The search functions read the items of the range without checking them, so a
 * caller that takes any of these arguments from a file header or from its own
 * input calls this first. The bindings of this library call it for every search.
 *
 * The last item that is read is (last - 1), at the byte offset
 * ((blklen * (last - 1)) + blkpos), and it occupies tsize bytes. The comparison
 * is written as a division because that product overflows for the values a
 * corrupted header can hold.
 *
 * @param avail   Number of bytes available from the offset of the first block.
 * @param blklen  Length of the binary block in bytes.
 * @param blkpos  Byte offset of the value inside a binary block.
 * @param last    Item past the last one of the search range.
 * @param tsize   Size of the searched type in bytes.
 *
 * @return true when every item of the range is inside the available bytes.
 */
static inline bool binsearch_check_row_range(uint64_t avail, uint64_t blklen, uint64_t blkpos, uint64_t last, uint64_t tsize)
{
    if (last == 0) // no item is read
    {
        return true;
    }
    if ((avail < tsize) || (blkpos > (avail - tsize)))
    {
        return false;
    }
    if (blklen == 0) // every item is at the same address
    {
        return true;
    }
    return (bool)((last - 1) <= (((avail - tsize) - blkpos) / blklen));
}

/**
 * Check that every item of a column mode search is inside a block of memory.
 *
 * The column mode functions read the values through a typed pointer, so the
 * offset of the column must be a multiple of the size of its type as well:
 * binsearch_check_col_offset reports that part.
 *
 * @param avail  Number of bytes available from the offset of the first item.
 * @param last   Item past the last one of the search range.
 * @param tsize  Size of the searched type in bytes.
 *
 * @return true when every item of the range is inside the available bytes.
 */
static inline bool binsearch_check_col_range(uint64_t avail, uint64_t last, uint64_t tsize)
{
    return (bool)(last <= (avail / tsize));
}

/**
 * Check that a column offset can be read through a pointer to its type.
 *
 * @param offset Byte offset of the first item of the column.
 * @param tsize  Size of the column type in bytes.
 *
 * @return true when the offset is a multiple of the size of the type.
 */
static inline bool binsearch_check_col_offset(uint64_t offset, uint64_t tsize)
{
    return (bool)((offset % tsize) == 0);
}

/**
 * Check that the bits a "_sub_" search matches are inside the searched type.
 *
 * @param bitstart First bit to match, counted from the most significant bit.
 * @param bitend   Last bit to match, counted from the most significant bit.
 * @param tsize    Size of the searched type in bytes.
 *
 * @return true when bitstart <= bitend < (8 * tsize).
 */
static inline bool binsearch_check_bits(uint8_t bitstart, uint8_t bitend, uint64_t tsize)
{
    return (bool)((bitstart <= bitend) && ((uint64_t)bitend < (tsize * 8)));
}

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
 * Return a typed pointer to the given byte offset.
 *
 * The offset must be a multiple of the size of T, as required by the column
 * mode functions. The cast cannot express that: on a target that rejects a
 * misaligned load the alignment is guaranteed by the caller, and by
 * check_col_bounds for the offsets that come from a file header.
 *
 * @param T        Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t.
 * @param src      Memory mapped file address.
 * @param offset   Byte offset.
 *
 * @return Pointer to T at the given offset.
 */
#define get_src_offset(T, src, offset) ((const T *)((src) + (offset)))

/**
 * Byte order of the values stored in a data block.
 */
typedef enum binsearch_order_t
{
    BINSEARCH_ORDER_LE = 0, //!< The values are little-endian: use the "_le_" functions.
    BINSEARCH_ORDER_BE = 1, //!< The values are big-endian: use the "_be_" functions.
} binsearch_order_t;

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
    uint8_t  border;            //!< Byte order of the values, as a binsearch_order_t. No format this recognises declares one in a part of its header that is read here, so mmap_binfile always sets it to BINSEARCH_ORDER_LE. An Apache Arrow file carries its byte order in the schema flatbuffer, which is skipped over rather than parsed, so a big-endian Arrow file is reported as little-endian. Overwrite the field when the content is known to be big-endian.
    uint8_t  ctbytes[MAXCOLS];  //!< Size in bytes of each column type (1, 2, 4 or 8). Set by the caller except for the BINSRC1 format.
    uint64_t index[MAXCOLS];    //!< Byte offset of the first item of each column.
    binsearch_prefetch_t prefetch; //!< Whether the find_many functions ask for the pages of a batch before reading them. Set to BINSEARCH_PREFETCH_AUTO by mmap_binfile. The C functions do not read this field: it is where a caller keeps the value to pass to them, and what the Go and Python wrappers pass.
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

// Read the item of the given index. GET_ROW_ITEM reads it from a binary block,
// GET_COL_ITEM from a contiguous array, and the SUB variants extract the bits
// from bitstart to bitend and right-align them.

#define GET_ROW_ITEM(O, T, IDX) bytes_##O##_to_##T(src, get_address(blklen, blkpos, (IDX)))

#define GET_COL_ITEM(O, T, IDX) order_##O##_##T(*(src + (IDX)))

// Address of item IDX inside the mapping, for the page requests of find_many.
// The _SUB_ searches read the same item as the plain ones.

#define ADDR_ROW_ITEM(T, IDX) ((src) + get_address(blklen, blkpos, (IDX)))

#define ADDR_COL_ITEM(T, IDX) ((const uint8_t *)((src) + (IDX)))

#define ADDR_ROW_SUB_ITEM(T, IDX) ADDR_ROW_ITEM(T, (IDX))

#define ADDR_COL_SUB_ITEM(T, IDX) ADDR_COL_ITEM(T, (IDX))

#define GET_ROW_SUB_ITEM(O, T, IDX) ((T)((GET_ROW_ITEM(O, T, IDX) >> rshift) & bitmask))

#define GET_COL_SUB_ITEM(O, T, IDX) ((T)((GET_COL_ITEM(O, T, IDX) >> rshift) & bitmask))

// The search range is kept in local variables instead of being read back
// through "first" and "last" on every step: both point to uint64_t and may
// alias each other, and in row mode "src" points to uint8_t and may alias
// anything, so a compiler has to reload them after each write.
#define FIND_VARS(T) \
    const uint64_t firstorig = *first; \
    const uint64_t lastorig = *last; \
    uint64_t base = firstorig; \
    uint64_t n = ((lastorig > base) ? (lastorig - base) : 0); \
    uint64_t half = 0; \
    T x = 0;

// Move "base" to the first item of the range that is not less than the searched
// value. The step is added with a conditional expression rather than taken in a
// branch, so that a compiler emits a conditional move: the branch of a plain
// binary search is unpredictable and costs a misprediction on every step.
//
// Every probe is inside the range the caller asked for: "half" is at least 1
// and at most n/2, and (base + n) never exceeds the end of the range.
#define FIND_FIRST_LOOP(GET, O, T) \
    while (n > 1) \
    { \
        half = (n >> 1); \
        x = GET(O, T, ((base + half) - 1)); \
        base += ((x < search) ? half : 0); \
        n -= half; \
    } \
    if ((n == 1) && (GET(O, T, base) < search)) \
    { \
        ++base; \
    }

// Same as FIND_FIRST_LOOP, moving "base" past the last item that is not greater
// than the searched value.
#define FIND_LAST_LOOP(GET, O, T) \
    while (n > 1) \
    { \
        half = (n >> 1); \
        x = GET(O, T, ((base + half) - 1)); \
        base += ((x > search) ? 0 : half); \
        n -= half; \
    } \
    if ((n == 1) && !(GET(O, T, base) > search)) \
    { \
        ++base; \
    }

// The position the loop converged to, clamped to the end of the range the
// caller asked for: the loop leaves base above it when the range was empty.
#define FIND_NEW_LAST ((lastorig > base) ? base : lastorig)

// Write back the narrowed range. On the not-found path "first" is the item that
// precedes the insertion point, so it can be one position below the range the
// caller passed in, and "last" is the insertion point.
#define FIND_RETURN_NOT_FOUND \
    *first = ((base > 0) ? (base - 1) : 0); \
    *last = FIND_NEW_LAST; \
    return lastorig;

// Inspect the item the loop converged to. It is only read when it is inside the
// range the caller asked for: the loop leaves base == lastorig when the searched
// value is above the range.
#define FIND_FIRST_TAIL(GET, O, T) \
    if (base < lastorig) \
    { \
        x = GET(O, T, base); \
        if (x == search) \
        { \
            *first = base; \
            *last = FIND_NEW_LAST; \
            return base; \
        } \
    } \
    FIND_RETURN_NOT_FOUND

// Inspect the item that precedes the position the loop converged to. It is only
// read when that position is above the first item of the range the caller asked
// for, which also rules out the wrap-around of the decrement when the range
// starts at 0.
#define FIND_LAST_TAIL(GET, O, T) \
    if (base > firstorig) \
    { \
        const uint64_t middle = (base - 1); \
        x = GET(O, T, middle); \
        if (x == search) \
        { \
            *first = base; \
            *last = FIND_NEW_LAST; \
            return middle; \
        } \
    } \
    FIND_RETURN_NOT_FOUND

#define SUB_ITEM_VARS(T) \
    T bitmask = ((T)1 << (bitend - bitstart)); \
    bitmask ^= (bitmask - 1); \
    const uint8_t rshift = (((uint8_t)(sizeof(T) * 8) - 1) - bitend);

// --- RANGE ---

// Narrow the range to the items equal to the searched value, in one descent.
//
// A find_first and a find_last over the same range walk the same probes until
// the first one that equals the searched value, so the two searches are run as
// one until they part there. Below that probe only the lower bound is left to
// find, above it only the upper one, and each of the two tails is a plain
// branchless step of the loop it came from.
//
// The descent tests for equality first, so reaching the step that moves the
// window down means the probe was strictly greater than the searched value.
// That is what keeps the upper bound inside the window: the window only shrinks
// from above past an item that is greater than everything being searched for.
#define FIND_RANGE_VARS(T) \
    const uint64_t firstorig = *first; \
    const uint64_t lastorig = *last; \
    uint64_t base = firstorig; \
    uint64_t n = ((lastorig > base) ? (lastorig - base) : 0); \
    uint64_t half = 0; \
    uint64_t probe = 0; \
    T x = 0;

#define FIND_RANGE_BODY(GET, O, T) \
    while (n > 1) \
    { \
        half = (n >> 1); \
        probe = ((base + half) - 1); \
        x = GET(O, T, probe); \
        if (x == search) \
        { \
            uint64_t lo = base; \
            uint64_t lon = ((probe - base) + 1); \
            uint64_t hi = (probe + 1); \
            uint64_t hin = ((base + n) - hi); \
            /* The lower bound needs no step after its loop, unlike
               FIND_FIRST_LOOP. Its window ends on the probe, which is equal to
               the searched value, and every step keeps the last item of the
               window at or above that value: the step that moves the window up
               leaves the last item where it is, and the step that shrinks it
               from above stops on an item at or after a probe that was not
               below the searched value. So the item the loop converges to is
               the bound, and never the one below it. */ \
            while (lon > 1) \
            { \
                const uint64_t lh = (lon >> 1); \
                lo += ((GET(O, T, ((lo + lh) - 1)) < search) ? lh : 0); \
                lon -= lh; \
            } \
            /* The upper bound does need one: its window starts after the probe,
               so nothing inside it is known to be above the searched value. */ \
            while (hin > 1) \
            { \
                const uint64_t hh = (hin >> 1); \
                hi += ((GET(O, T, ((hi + hh) - 1)) > search) ? 0 : hh); \
                hin -= hh; \
            } \
            if (!(GET(O, T, hi) > search)) \
            { \
                ++hi; \
            } \
            *first = lo; \
            *last = hi; \
            return (hi - lo); \
        } \
        base += ((x < search) ? half : 0); \
        n -= half; \
    } \
    if (n == 1) \
    { \
        x = GET(O, T, base); \
        if (x == search) \
        { \
            *first = base; \
            *last = (base + 1); \
            return 1; \
        } \
        if (x < search) \
        { \
            ++base; \
        } \
    } \
    *first = FIND_NEW_LAST; \
    *last = FIND_NEW_LAST; \
    return 0;

// --- BATCH ---

// Search "count" values over the same range, advancing every search of the
// batch by one step before any of them takes the next one.
//
// A binary search over data larger than the cache is a chain of dependent
// misses: the address of each probe comes out of the previous load, so one miss
// is outstanding at a time and the cost is the full latency of each. The
// searches of a batch are independent of each other, so interleaving them puts
// that many misses in flight together.
//
// Every search of the batch covers the same range, so they all take the same
// number of steps and the inner loop over the batch has a constant trip count.
//
// A count that is not a multiple of BINSEARCH_BATCH leaves a remainder of two or
// more values, which is copied into bmtail and padded with its last value up to
// a full batch. The padding lanes repeat a search that is already in the batch,
// so they walk the same path and touch no page the batch does not touch anyway,
// and one descent replaces the remainder searches that would otherwise run one
// at a time. A remainder of exactly one value is not worth a full batch and is
// searched on its own.
// When the pages of a batch are not resident, the overlap the batch is built
// for does not happen: the first lane to touch a missing page traps into the
// kernel and the other lanes cannot issue their reads until it returns, so the
// faults are served one at a time. Asking for every lane's page with
// BINSEARCH_WILLNEED before any of them is read puts the whole batch in the
// device queue at once, which is worth about an order of magnitude on a file
// too large for RAM, and costs a system call per value per step on a file that
// fits, where it buys nothing.
//
// BINSEARCH_PREFETCH_AUTO therefore times one batch out of BINSEARCH_PROBE_EVERY
// with the request turned off, and turns it on for the batches in between when
// that one took more than BINSEARCH_SLOW_NS per value. A resident batch costs
// three orders of magnitude less than that, so the threshold sits in a wide gap.
// Probing again rather than deciding once follows a mapping whose residency
// changes while it is being searched.
#define FIND_MANY_VARS(T) \
    const uint64_t norig = ((last > first) ? (last - first) : 0); \
    uint64_t base[BINSEARCH_BATCH]; \
    T bmtail[BINSEARCH_BATCH]; \
    uint64_t bmtailpos[BINSEARCH_BATCH]; \
    uint64_t rem = 0; \
    uint64_t i = 0; \
    uint64_t j = 0; \
    uint64_t n = 0; \
    uint64_t half = 0; \
    FIND_MANY_PREFETCH_VARS

#if BINSEARCH_HAVE_WILLNEED

#define FIND_MANY_PREFETCH_VARS \
    uint64_t bmpgsz = 0; \
    uint64_t bmblk = 0; \
    int bmwn = (prefetch == BINSEARCH_PREFETCH_ALWAYS);

// Ask for the page holding item IDX. ADDR gives its address inside the mapping.
#define FIND_MANY_WILLNEED(ADDR, T, IDX) \
    { \
        const uintptr_t bma = (uintptr_t)(ADDR(T, (IDX))); \
        (void)BINSEARCH_WILLNEED((void *)(bma & ~(uintptr_t)(bmpgsz - 1)), (size_t)bmpgsz); \
    }

#define FIND_MANY_PREFETCH_STEP(ADDR, T) \
    if (bmwn) \
    { \
        for (j = 0; j < BINSEARCH_BATCH; j++) \
        { \
            FIND_MANY_WILLNEED(ADDR, T, ((base[j] + half) - 1)) \
        } \
    }

#define FIND_MANY_PREFETCH_START \
    if (bmwn) \
    { \
        bmpgsz = binsearch_page_size(); \
    }

#define FIND_MANY_PROBE_BEGIN \
    const int bmprobe = ((prefetch == BINSEARCH_PREFETCH_AUTO) && ((bmblk & (BINSEARCH_PROBE_EVERY - 1)) == 0)); \
    uint64_t bmt0 = 0; \
    if (bmprobe) \
    { \
        bmwn = 0; \
        bmt0 = binsearch_now_ns(); \
    }

#define FIND_MANY_PROBE_END \
    if (bmprobe) \
    { \
        bmwn = ((binsearch_now_ns() - bmt0) > ((uint64_t)BINSEARCH_SLOW_NS * BINSEARCH_BATCH)); \
        FIND_MANY_PREFETCH_START \
    } \
    bmblk++;

#else

// The platform cannot ask for a page, so none of the prefetch code is generated
// and the mode the caller passes is ignored.

#define FIND_MANY_PREFETCH_VARS (void)prefetch;
#define FIND_MANY_PREFETCH_STEP(ADDR, T)
#define FIND_MANY_PREFETCH_START
#define FIND_MANY_PROBE_BEGIN
#define FIND_MANY_PROBE_END

#endif

// One lockstep descent over BINSEARCH_BATCH values, followed by the step that
// FIND_FIRST_LOOP takes when a single item is left and the hit test of
// FIND_FIRST_TAIL. SRCH and POSA are the bases the batch reads and writes.
#define FIND_MANY_BLOCK(GET, ADDR, O, T, SRCH, POSA) \
    for (j = 0; j < BINSEARCH_BATCH; j++) \
    { \
        base[j] = first; \
    } \
    n = norig; \
    while (n > 1) \
    { \
        half = (n >> 1); \
        FIND_MANY_PREFETCH_STEP(ADDR, T) \
        for (j = 0; j < BINSEARCH_BATCH; j++) \
        { \
            const T x = GET(O, T, ((base[j] + half) - 1)); \
            base[j] += ((x < (SRCH)[j]) ? half : 0); \
        } \
        n -= half; \
    } \
    for (j = 0; j < BINSEARCH_BATCH; j++) \
    { \
        if ((n == 1) && (GET(O, T, base[j]) < (SRCH)[j])) \
        { \
            ++base[j]; \
        } \
        (POSA)[j] = last; \
        if (base[j] < last) \
        { \
            const T x = GET(O, T, base[j]); \
            if (x == (SRCH)[j]) \
            { \
                (POSA)[j] = base[j]; \
            } \
        } \
    }

// Run one descent per full batch, then one more over the padded remainder. On
// return "i" is the number of values searched, which is "count" unless a single
// value is left over.
#define FIND_MANY_LOOP(GET, ADDR, O, T) \
    FIND_MANY_PREFETCH_START \
    for (i = 0; ((i + BINSEARCH_BATCH) <= count); i += BINSEARCH_BATCH) \
    { \
        FIND_MANY_PROBE_BEGIN \
        FIND_MANY_BLOCK(GET, ADDR, O, T, (search + i), (pos + i)) \
        FIND_MANY_PROBE_END \
    } \
    rem = (count - i); \
    if (rem > 1) \
    { \
        for (j = 0; j < BINSEARCH_BATCH; j++) \
        { \
            bmtail[j] = search[i + ((j < rem) ? j : (rem - 1))]; \
        } \
        FIND_MANY_BLOCK(GET, ADDR, O, T, bmtail, bmtailpos) \
        for (j = 0; j < rem; j++) \
        { \
            pos[i + j] = bmtailpos[j]; \
        } \
        i = count; \
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

#define COL_HAS_END_BLOCK(O, T) \
    return (order_##O##_##T(*(src + *pos)) == search);

#define HAS_SUB_END_BLOCK(O, T) \
    return (((GET_POS_BLOCK(O, T) >> rshift) & bitmask) == search);

#define COL_HAS_SUB_END_BLOCK(O, T) \
    return (((order_##O##_##T(*(src + *pos)) >> rshift) & bitmask) == search);

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
@param first     Pointer to the first item of the search range (min value = 0). On return it is the lower bound of the searched value, or the position below it when the value is absent, which can be one position below the range that was passed in.
@param last      Pointer to the item past the last one of the search range (max value = nrows). On return it is the lower bound of the searched value: the item that was found, or the position where it would be inserted. It is not the bound to pass to has_next: that one takes the end of the original range.
@param search    Value to search.
@return The item number when the value is found, or the initial value of "last" when it is not.
 */ \
static inline uint64_t find_first_##O##_##T(const uint8_t *src, uint64_t blklen, uint64_t blkpos, uint64_t *first, uint64_t *last, T search) \
{ \
FIND_VARS(T) \
FIND_FIRST_LOOP(GET_ROW_ITEM, O, T) \
FIND_FIRST_TAIL(GET_ROW_ITEM, O, T) \
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
 * Define a function that searches for the first item whose bits from bitstart
 * to bitend equal a value, in a memory mapped file of adjacent blocks of sorted
 * binary data.
 *
 * @param O Byte order: be or le.
 * @param T Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t.
 */
#define define_find_first_sub(O, T) \
/** Search for the first item whose bits from bitstart to bitend equal a T
value, in a memory mapped file of adjacent blocks of sorted binary data.
The values must be encoded in "O" byte order and sorted in ascending order.
@param src       Memory mapped file address.
@param blklen    Length of the binary block in bytes.
@param blkpos    Byte offset of the value inside a binary block.
@param bitstart  First bit to match, counted from the most significant bit of T (usually 0).
@param bitend    Last bit to match, counted from the most significant bit of T (7 for uint8_t, 15 for uint16_t, 31 for uint32_t, 63 for uint64_t). Must be at least bitstart and less than the width of T in bits.
@param first     Pointer to the first item of the search range (min value = 0). On return it is the lower bound of the searched value, or the position below it when the value is absent, which can be one position below the range that was passed in.
@param last      Pointer to the item past the last one of the search range (max value = nrows). On return it is the lower bound of the searched value: the item that was found, or the position where it would be inserted. It is not the bound to pass to has_next: that one takes the end of the original range.
@param search    Value to search.
@return The item number when the value is found, or the initial value of "last" when it is not.
 */ \
static inline uint64_t find_first_sub_##O##_##T(const uint8_t *src, uint64_t blklen, uint64_t blkpos, uint8_t bitstart, uint8_t bitend, uint64_t *first, uint64_t *last, T search) \
{ \
SUB_ITEM_VARS(T) \
FIND_VARS(T) \
FIND_FIRST_LOOP(GET_ROW_SUB_ITEM, O, T) \
FIND_FIRST_TAIL(GET_ROW_SUB_ITEM, O, T) \
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
 * @param T Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t.
 */
#define define_find_last(O, T) \
/** Search for the last occurrence of a T value in a memory mapped file of
adjacent blocks of sorted binary data.
The values must be encoded in "O" byte order and sorted in ascending order.
@param src       Memory mapped file address.
@param blklen    Length of the binary block in bytes.
@param blkpos    Byte offset of the value inside a binary block.
@param first     Pointer to the first item of the search range (min value = 0). On return it is the upper bound of the searched value, or the position below it when the value is absent, which can be one position below the range that was passed in.
@param last      Pointer to the item past the last one of the search range (max value = nrows). On return it is the upper bound of the searched value: the position after the item that was found, or the position where it would be inserted. It is not the bound to pass to has_next: that one takes the end of the original range.
@param search    Value to search.
@return The item number when the value is found, or the initial value of "last" when it is not.
 */ \
static inline uint64_t find_last_##O##_##T(const uint8_t *src, uint64_t blklen, uint64_t blkpos, uint64_t *first, uint64_t *last, T search) \
{ \
FIND_VARS(T) \
FIND_LAST_LOOP(GET_ROW_ITEM, O, T) \
FIND_LAST_TAIL(GET_ROW_ITEM, O, T) \
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
 * Define a function that searches for the last item whose bits from bitstart to
 * bitend equal a value, in a memory mapped file of adjacent blocks of sorted
 * binary data.
 *
 * @param O Byte order: be or le.
 * @param T Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t.
 */
#define define_find_last_sub(O, T) \
/** Search for the last item whose bits from bitstart to bitend equal a T
value, in a memory mapped file of adjacent blocks of sorted binary data.
The values must be encoded in "O" byte order and sorted in ascending order.
@param src       Memory mapped file address.
@param blklen    Length of the binary block in bytes.
@param blkpos    Byte offset of the value inside a binary block.
@param bitstart  First bit to match, counted from the most significant bit of T (usually 0).
@param bitend    Last bit to match, counted from the most significant bit of T (7 for uint8_t, 15 for uint16_t, 31 for uint32_t, 63 for uint64_t). Must be at least bitstart and less than the width of T in bits.
@param first     Pointer to the first item of the search range (min value = 0). On return it is the upper bound of the searched value, or the position below it when the value is absent, which can be one position below the range that was passed in.
@param last      Pointer to the item past the last one of the search range (max value = nrows). On return it is the upper bound of the searched value: the position after the item that was found, or the position where it would be inserted. It is not the bound to pass to has_next: that one takes the end of the original range.
@param search    Value to search.
@return The item number when the value is found, or the initial value of "last" when it is not.
 */ \
static inline uint64_t find_last_sub_##O##_##T(const uint8_t *src, uint64_t blklen, uint64_t blkpos, uint8_t bitstart, uint8_t bitend, uint64_t *first, uint64_t *last, T search) \
{ \
SUB_ITEM_VARS(T) \
FIND_VARS(T) \
FIND_LAST_LOOP(GET_ROW_SUB_ITEM, O, T) \
FIND_LAST_TAIL(GET_ROW_SUB_ITEM, O, T) \
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
 * Define a function that narrows a range to the items equal to a value
 * in a memory mapped file of adjacent blocks of sorted binary data.
 *
 * @param O Byte order: be or le.
 * @param T Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t.
 */
#define define_find_range(O, T) \
/** Narrow a range to the items equal to a T value in a memory mapped file of
adjacent blocks of sorted binary data.
The values must be encoded in "O" byte order and sorted in ascending order.
It computes in one descent what find_first and find_last compute in two, which
costs about half of the two searches on data larger than the cache.
@param src       Memory mapped file address.
@param blklen    Length of the binary block in bytes.
@param blkpos    Byte offset of the value inside a binary block.
@param first     Pointer to the first item of the search range (min value = 0). On return it is the first item equal to the searched value, or the position where it would be inserted when it is absent.
@param last      Pointer to the item past the last one of the search range (max value = nrows). On return it is the item past the last one equal to the searched value, or the position where it would be inserted when it is absent.
@param search    Value to search.
@return The number of items equal to the searched value, which is 0 when it is absent.
 */ \
static inline uint64_t find_range_##O##_##T(const uint8_t *src, uint64_t blklen, uint64_t blkpos, uint64_t *first, uint64_t *last, T search) \
{ \
FIND_RANGE_VARS(T) \
FIND_RANGE_BODY(GET_ROW_ITEM, O, T) \
}

define_find_range(be, uint8_t)
define_find_range(be, uint16_t)
define_find_range(be, uint32_t)
define_find_range(be, uint64_t)
define_find_range(le, uint8_t)
define_find_range(le, uint16_t)
define_find_range(le, uint32_t)
define_find_range(le, uint64_t)

/**
 * Define a function that narrows a range to the items whose bits from bitstart
 * to bitend equal a value, in a memory mapped file of adjacent blocks of sorted
 * binary data.
 *
 * @param O Byte order: be or le.
 * @param T Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t.
 */
#define define_find_range_sub(O, T) \
/** Narrow a range to the items whose bits from bitstart to bitend equal a T
value, in a memory mapped file of adjacent blocks of sorted binary data.
The values must be encoded in "O" byte order and sorted in ascending order.
@param src       Memory mapped file address.
@param blklen    Length of the binary block in bytes.
@param blkpos    Byte offset of the value inside a binary block.
@param bitstart  First bit to match, counted from the most significant bit of T (usually 0).
@param bitend    Last bit to match, counted from the most significant bit of T (7 for uint8_t, 15 for uint16_t, 31 for uint32_t, 63 for uint64_t). Must be at least bitstart and less than the width of T in bits.
@param first     Pointer to the first item of the search range (min value = 0). On return it is the first item equal to the searched value, or the position where it would be inserted when it is absent.
@param last      Pointer to the item past the last one of the search range (max value = nrows). On return it is the item past the last one equal to the searched value, or the position where it would be inserted when it is absent.
@param search    Value to search.
@return The number of items equal to the searched value, which is 0 when it is absent.
 */ \
static inline uint64_t find_range_sub_##O##_##T(const uint8_t *src, uint64_t blklen, uint64_t blkpos, uint8_t bitstart, uint8_t bitend, uint64_t *first, uint64_t *last, T search) \
{ \
SUB_ITEM_VARS(T) \
FIND_RANGE_VARS(T) \
FIND_RANGE_BODY(GET_ROW_SUB_ITEM, O, T) \
}

define_find_range_sub(be, uint8_t)
define_find_range_sub(be, uint16_t)
define_find_range_sub(be, uint32_t)
define_find_range_sub(be, uint64_t)
define_find_range_sub(le, uint8_t)
define_find_range_sub(le, uint16_t)
define_find_range_sub(le, uint32_t)
define_find_range_sub(le, uint64_t)

/**
 * Define a function that searches several values at once
 * in a memory mapped file of adjacent blocks of sorted binary data.
 *
 * @param O Byte order: be or le.
 * @param T Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t.
 */
#define define_find_many(O, T) \
/** Search for the first occurrence of each of several T values in a memory
mapped file of adjacent blocks of sorted binary data.
The values must be encoded in "O" byte order and sorted in ascending order.
Every value is searched over the same range and reports what the matching
find_first function reports for it, but the searches are advanced in lockstep so
that their cache misses overlap, which is about three to four times as fast per
value on data larger than the CPU cache and still held in RAM.
@param src       Memory mapped file address.
@param blklen    Length of the binary block in bytes.
@param blkpos    Byte offset of the value inside a binary block.
@param first     First item of the search range (min value = 0).
@param last      Item past the last one of the search range (max value = nrows).
@param search    Values to search.
@param pos       Array of "count" items that receives, for each searched value, the item number when it is found or "last" when it is not.
@param count     Number of values to search.
@param prefetch  Whether to ask for the pages of a batch before reading them: BINSEARCH_PREFETCH_AUTO to ask only when a timed batch is slow enough to be reading from a device.
 */ \
static inline void find_many_##O##_##T(const uint8_t *src, uint64_t blklen, uint64_t blkpos, uint64_t first, uint64_t last, const T *search, uint64_t *pos, uint64_t count, binsearch_prefetch_t prefetch) \
{ \
FIND_MANY_VARS(T) \
FIND_MANY_LOOP(GET_ROW_ITEM, ADDR_ROW_ITEM, O, T) \
if (i < count) \
{ \
    uint64_t f = first; \
    uint64_t l = last; \
    pos[i] = find_first_##O##_##T(src, blklen, blkpos, &f, &l, search[i]); \
} \
}

define_find_many(be, uint8_t)
define_find_many(be, uint16_t)
define_find_many(be, uint32_t)
define_find_many(be, uint64_t)
define_find_many(le, uint8_t)
define_find_many(le, uint16_t)
define_find_many(le, uint32_t)
define_find_many(le, uint64_t)

/**
 * Define a function that searches the bits from bitstart to bitend of several
 * values at once, in a memory mapped file of adjacent blocks of sorted binary
 * data.
 *
 * @param O Byte order: be or le.
 * @param T Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t.
 */
#define define_find_many_sub(O, T) \
/** Search for the first occurrence of each of several T values, matching only
the bits from bitstart to bitend, in a memory mapped file of adjacent blocks of
sorted binary data.
The values must be encoded in "O" byte order and sorted in ascending order.
@param src       Memory mapped file address.
@param blklen    Length of the binary block in bytes.
@param blkpos    Byte offset of the value inside a binary block.
@param bitstart  First bit to match, counted from the most significant bit of T (usually 0).
@param bitend    Last bit to match, counted from the most significant bit of T (7 for uint8_t, 15 for uint16_t, 31 for uint32_t, 63 for uint64_t). Must be at least bitstart and less than the width of T in bits.
@param first     First item of the search range (min value = 0).
@param last      Item past the last one of the search range (max value = nrows).
@param search    Values to search.
@param pos       Array of "count" items that receives, for each searched value, the item number when it is found or "last" when it is not.
@param count     Number of values to search.
@param prefetch  Whether to ask for the pages of a batch before reading them: BINSEARCH_PREFETCH_AUTO to ask only when a timed batch is slow enough to be reading from a device.
 */ \
static inline void find_many_sub_##O##_##T(const uint8_t *src, uint64_t blklen, uint64_t blkpos, uint8_t bitstart, uint8_t bitend, uint64_t first, uint64_t last, const T *search, uint64_t *pos, uint64_t count, binsearch_prefetch_t prefetch) \
{ \
SUB_ITEM_VARS(T) \
FIND_MANY_VARS(T) \
FIND_MANY_LOOP(GET_ROW_SUB_ITEM, ADDR_ROW_SUB_ITEM, O, T) \
if (i < count) \
{ \
    uint64_t f = first; \
    uint64_t l = last; \
    pos[i] = find_first_sub_##O##_##T(src, blklen, blkpos, bitstart, bitend, &f, &l, search[i]); \
} \
}

define_find_many_sub(be, uint8_t)
define_find_many_sub(be, uint16_t)
define_find_many_sub(be, uint32_t)
define_find_many_sub(be, uint64_t)
define_find_many_sub(le, uint8_t)
define_find_many_sub(le, uint16_t)
define_find_many_sub(le, uint32_t)
define_find_many_sub(le, uint64_t)

/**
 * Define a function that checks whether the next item matches the search value.
 *
 * @param O Byte order: be or le.
 * @param T Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t.
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
@param last      Item past the last one of the original search range (max value = nrows), not the value the find_first function wrote into its "last" argument.
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
 * Define a function that checks whether the bits from bitstart to bitend of the
 * next item match the search value.
 *
 * @param O Byte order: be or le.
 * @param T Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t.
 */
#define define_has_next_sub(O, T) \
/** Check whether the bits from bitstart to bitend of the item after "pos", in a
memory mapped file of adjacent blocks of sorted binary data, match the search
value.
The values must be encoded in "O" byte order and sorted in ascending order.
Set "pos" to the item returned by the matching find_first_sub function to walk
forward over the items that satisfy the search.
@param src       Memory mapped file address.
@param blklen    Length of the binary block in bytes.
@param blkpos    Byte offset of the value inside a binary block.
@param bitstart  First bit to match, counted from the most significant bit of T (usually 0).
@param bitend    Last bit to match, counted from the most significant bit of T (7 for uint8_t, 15 for uint16_t, 31 for uint32_t, 63 for uint64_t). Must be at least bitstart and less than the width of T in bits.
@param pos       Pointer to the current item. Updated to the next item.
@param last      Item past the last one of the original search range (max value = nrows), not the value the find_first function wrote into its "last" argument.
@param search    Value to search.
@return true when the bits of the next item match the search value, false otherwise.
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
 * @param T Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t.
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
@param first     First item of the original search range (min value = 0), not the value the find_last function wrote into its "first" argument.
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
 * Define a function that checks whether the bits from bitstart to bitend of the
 * previous item match the search value.
 *
 * @param O Byte order: be or le.
 * @param T Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t.
 */
#define define_has_prev_sub(O, T) \
/** Check whether the bits from bitstart to bitend of the item before "pos", in
a memory mapped file of adjacent blocks of sorted binary data, match the search
value.
The values must be encoded in "O" byte order and sorted in ascending order.
Set "pos" to the item returned by the matching find_last_sub function to walk
backward over the items that satisfy the search.
@param src       Memory mapped file address.
@param blklen    Length of the binary block in bytes.
@param blkpos    Byte offset of the value inside a binary block.
@param bitstart  First bit to match, counted from the most significant bit of T (usually 0).
@param bitend    Last bit to match, counted from the most significant bit of T (7 for uint8_t, 15 for uint16_t, 31 for uint32_t, 63 for uint64_t). Must be at least bitstart and less than the width of T in bits.
@param first     First item of the original search range (min value = 0), not the value the find_last function wrote into its "first" argument.
@param pos       Pointer to the current item. Updated to the previous item.
@param search    Value to search.
@return true when the bits of the previous item match the search value, false otherwise.
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
 * @param O Byte order: be or le.
 * @param T Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t.
 */
#define define_col_find_first(O, T) \
/** Search for the first occurrence of a T value in a contiguous array of
unsigned integers of the same type.
The values must be encoded in "O" byte order and sorted in ascending order.
@param src       Pointer to the array of values.
@param first     Pointer to the first item of the search range (min value = 0). On return it is the lower bound of the searched value, or the position below it when the value is absent, which can be one position below the range that was passed in.
@param last      Pointer to the item past the last one of the search range (max value = nrows). On return it is the lower bound of the searched value: the item that was found, or the position where it would be inserted. It is not the bound to pass to has_next: that one takes the end of the original range.
@param search    Value to search.
@return The item number when the value is found, or the initial value of "last" when it is not.
 */ \
static inline uint64_t col_find_first_##O##_##T(const T *src, uint64_t *first, uint64_t *last, T search) \
{ \
FIND_VARS(T) \
FIND_FIRST_LOOP(GET_COL_ITEM, O, T) \
FIND_FIRST_TAIL(GET_COL_ITEM, O, T) \
}

define_col_find_first(be, uint8_t)
define_col_find_first(be, uint16_t)
define_col_find_first(be, uint32_t)
define_col_find_first(be, uint64_t)
define_col_find_first(le, uint8_t)
define_col_find_first(le, uint16_t)
define_col_find_first(le, uint32_t)
define_col_find_first(le, uint64_t)

/**
 * Define a function that searches for the first item whose bits from bitstart
 * to bitend equal a value, in a contiguous array of unsigned integers of the
 * same type.
 *
 * @param O Byte order: be or le.
 * @param T Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t.
 */
#define define_col_find_first_sub(O, T) \
/** Search for the first item whose bits from bitstart to bitend equal a T
value, in a contiguous array of unsigned integers of the same type.
The values must be encoded in "O" byte order and sorted in ascending order.
@param src       Pointer to the array of values.
@param bitstart  First bit to match, counted from the most significant bit of T (usually 0).
@param bitend    Last bit to match, counted from the most significant bit of T (7 for uint8_t, 15 for uint16_t, 31 for uint32_t, 63 for uint64_t). Must be at least bitstart and less than the width of T in bits.
@param first     Pointer to the first item of the search range (min value = 0). On return it is the lower bound of the searched value, or the position below it when the value is absent, which can be one position below the range that was passed in.
@param last      Pointer to the item past the last one of the search range (max value = nrows). On return it is the lower bound of the searched value: the item that was found, or the position where it would be inserted. It is not the bound to pass to has_next: that one takes the end of the original range.
@param search    Value to search.
@return The item number when the value is found, or the initial value of "last" when it is not.
 */ \
static inline uint64_t col_find_first_sub_##O##_##T(const T *src, uint8_t bitstart, uint8_t bitend, uint64_t *first, uint64_t *last, T search) \
{ \
SUB_ITEM_VARS(T) \
FIND_VARS(T) \
FIND_FIRST_LOOP(GET_COL_SUB_ITEM, O, T) \
FIND_FIRST_TAIL(GET_COL_SUB_ITEM, O, T) \
}

define_col_find_first_sub(be, uint8_t)
define_col_find_first_sub(be, uint16_t)
define_col_find_first_sub(be, uint32_t)
define_col_find_first_sub(be, uint64_t)
define_col_find_first_sub(le, uint8_t)
define_col_find_first_sub(le, uint16_t)
define_col_find_first_sub(le, uint32_t)
define_col_find_first_sub(le, uint64_t)

/**
 * Define a function that searches for the last occurrence of a value
 * in a contiguous array of unsigned integers of the same type.
 *
 * @param O Byte order: be or le.
 * @param T Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t.
 */
#define define_col_find_last(O, T) \
/** Search for the last occurrence of a T value in a contiguous array of
unsigned integers of the same type.
The values must be encoded in "O" byte order and sorted in ascending order.
@param src       Pointer to the array of values.
@param first     Pointer to the first item of the search range (min value = 0). On return it is the upper bound of the searched value, or the position below it when the value is absent, which can be one position below the range that was passed in.
@param last      Pointer to the item past the last one of the search range (max value = nrows). On return it is the upper bound of the searched value: the position after the item that was found, or the position where it would be inserted. It is not the bound to pass to has_next: that one takes the end of the original range.
@param search    Value to search.
@return The item number when the value is found, or the initial value of "last" when it is not.
 */ \
static inline uint64_t col_find_last_##O##_##T(const T *src, uint64_t *first, uint64_t *last, T search) \
{ \
FIND_VARS(T) \
FIND_LAST_LOOP(GET_COL_ITEM, O, T) \
FIND_LAST_TAIL(GET_COL_ITEM, O, T) \
}

define_col_find_last(be, uint8_t)
define_col_find_last(be, uint16_t)
define_col_find_last(be, uint32_t)
define_col_find_last(be, uint64_t)
define_col_find_last(le, uint8_t)
define_col_find_last(le, uint16_t)
define_col_find_last(le, uint32_t)
define_col_find_last(le, uint64_t)

/**
 * Define a function that searches for the last item whose bits from bitstart to
 * bitend equal a value, in a contiguous array of unsigned integers of the same
 * type.
 *
 * @param O Byte order: be or le.
 * @param T Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t.
 */
#define define_col_find_last_sub(O, T) \
/** Search for the last item whose bits from bitstart to bitend equal a T
value, in a contiguous array of unsigned integers of the same type.
The values must be encoded in "O" byte order and sorted in ascending order.
@param src       Pointer to the array of values.
@param bitstart  First bit to match, counted from the most significant bit of T (usually 0).
@param bitend    Last bit to match, counted from the most significant bit of T (7 for uint8_t, 15 for uint16_t, 31 for uint32_t, 63 for uint64_t). Must be at least bitstart and less than the width of T in bits.
@param first     Pointer to the first item of the search range (min value = 0). On return it is the upper bound of the searched value, or the position below it when the value is absent, which can be one position below the range that was passed in.
@param last      Pointer to the item past the last one of the search range (max value = nrows). On return it is the upper bound of the searched value: the position after the item that was found, or the position where it would be inserted. It is not the bound to pass to has_next: that one takes the end of the original range.
@param search    Value to search.
@return The item number when the value is found, or the initial value of "last" when it is not.
 */ \
static inline uint64_t col_find_last_sub_##O##_##T(const T *src, uint8_t bitstart, uint8_t bitend, uint64_t *first, uint64_t *last, T search) \
{ \
SUB_ITEM_VARS(T) \
FIND_VARS(T) \
FIND_LAST_LOOP(GET_COL_SUB_ITEM, O, T) \
FIND_LAST_TAIL(GET_COL_SUB_ITEM, O, T) \
}

define_col_find_last_sub(be, uint8_t)
define_col_find_last_sub(be, uint16_t)
define_col_find_last_sub(be, uint32_t)
define_col_find_last_sub(be, uint64_t)
define_col_find_last_sub(le, uint8_t)
define_col_find_last_sub(le, uint16_t)
define_col_find_last_sub(le, uint32_t)
define_col_find_last_sub(le, uint64_t)

/**
 * Define a function that narrows a range to the items equal to a value
 * in a contiguous array of unsigned integers of the same type.
 *
 * @param O Byte order: be or le.
 * @param T Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t.
 */
#define define_col_find_range(O, T) \
/** Narrow a range to the items equal to a T value in a contiguous array of
unsigned integers of the same type.
The values must be encoded in "O" byte order and sorted in ascending order.
It computes in one descent what col_find_first and col_find_last compute in two,
which costs about half of the two searches on data larger than the cache.
@param src       Pointer to the array of values.
@param first     Pointer to the first item of the search range (min value = 0). On return it is the first item equal to the searched value, or the position where it would be inserted when it is absent.
@param last      Pointer to the item past the last one of the search range (max value = nrows). On return it is the item past the last one equal to the searched value, or the position where it would be inserted when it is absent.
@param search    Value to search.
@return The number of items equal to the searched value, which is 0 when it is absent.
 */ \
static inline uint64_t col_find_range_##O##_##T(const T *src, uint64_t *first, uint64_t *last, T search) \
{ \
FIND_RANGE_VARS(T) \
FIND_RANGE_BODY(GET_COL_ITEM, O, T) \
}

define_col_find_range(be, uint8_t)
define_col_find_range(be, uint16_t)
define_col_find_range(be, uint32_t)
define_col_find_range(be, uint64_t)
define_col_find_range(le, uint8_t)
define_col_find_range(le, uint16_t)
define_col_find_range(le, uint32_t)
define_col_find_range(le, uint64_t)

/**
 * Define a function that narrows a range to the items whose bits from bitstart
 * to bitend equal a value, in a contiguous array of unsigned integers of the
 * same type.
 *
 * @param O Byte order: be or le.
 * @param T Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t.
 */
#define define_col_find_range_sub(O, T) \
/** Narrow a range to the items whose bits from bitstart to bitend equal a T
value, in a contiguous array of unsigned integers of the same type.
The values must be encoded in "O" byte order and sorted in ascending order.
@param src       Pointer to the array of values.
@param bitstart  First bit to match, counted from the most significant bit of T (usually 0).
@param bitend    Last bit to match, counted from the most significant bit of T (7 for uint8_t, 15 for uint16_t, 31 for uint32_t, 63 for uint64_t). Must be at least bitstart and less than the width of T in bits.
@param first     Pointer to the first item of the search range (min value = 0). On return it is the first item equal to the searched value, or the position where it would be inserted when it is absent.
@param last      Pointer to the item past the last one of the search range (max value = nrows). On return it is the item past the last one equal to the searched value, or the position where it would be inserted when it is absent.
@param search    Value to search.
@return The number of items equal to the searched value, which is 0 when it is absent.
 */ \
static inline uint64_t col_find_range_sub_##O##_##T(const T *src, uint8_t bitstart, uint8_t bitend, uint64_t *first, uint64_t *last, T search) \
{ \
SUB_ITEM_VARS(T) \
FIND_RANGE_VARS(T) \
FIND_RANGE_BODY(GET_COL_SUB_ITEM, O, T) \
}

define_col_find_range_sub(be, uint8_t)
define_col_find_range_sub(be, uint16_t)
define_col_find_range_sub(be, uint32_t)
define_col_find_range_sub(be, uint64_t)
define_col_find_range_sub(le, uint8_t)
define_col_find_range_sub(le, uint16_t)
define_col_find_range_sub(le, uint32_t)
define_col_find_range_sub(le, uint64_t)

/**
 * Define a function that searches several values at once
 * in a contiguous array of unsigned integers of the same type.
 *
 * @param O Byte order: be or le.
 * @param T Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t.
 */
#define define_col_find_many(O, T) \
/** Search for the first occurrence of each of several T values in a contiguous
array of unsigned integers of the same type.
The values must be encoded in "O" byte order and sorted in ascending order.
Every value is searched over the same range and reports what the matching
col_find_first function reports for it, but the searches are advanced in
lockstep so that their cache misses overlap, which is about three to four times
as fast per value on data larger than the CPU cache and still held in RAM.
@param src       Pointer to the array of values.
@param first     First item of the search range (min value = 0).
@param last      Item past the last one of the search range (max value = nrows).
@param search    Values to search.
@param pos       Array of "count" items that receives, for each searched value, the item number when it is found or "last" when it is not.
@param count     Number of values to search.
@param prefetch  Whether to ask for the pages of a batch before reading them: BINSEARCH_PREFETCH_AUTO to ask only when a timed batch is slow enough to be reading from a device.
 */ \
static inline void col_find_many_##O##_##T(const T *src, uint64_t first, uint64_t last, const T *search, uint64_t *pos, uint64_t count, binsearch_prefetch_t prefetch) \
{ \
FIND_MANY_VARS(T) \
FIND_MANY_LOOP(GET_COL_ITEM, ADDR_COL_ITEM, O, T) \
if (i < count) \
{ \
    uint64_t f = first; \
    uint64_t l = last; \
    pos[i] = col_find_first_##O##_##T(src, &f, &l, search[i]); \
} \
}

define_col_find_many(be, uint8_t)
define_col_find_many(be, uint16_t)
define_col_find_many(be, uint32_t)
define_col_find_many(be, uint64_t)
define_col_find_many(le, uint8_t)
define_col_find_many(le, uint16_t)
define_col_find_many(le, uint32_t)
define_col_find_many(le, uint64_t)

/**
 * Define a function that searches the bits from bitstart to bitend of several
 * values at once, in a contiguous array of unsigned integers of the same type.
 *
 * @param O Byte order: be or le.
 * @param T Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t.
 */
#define define_col_find_many_sub(O, T) \
/** Search for the first occurrence of each of several T values, matching only
the bits from bitstart to bitend, in a contiguous array of unsigned integers of
the same type.
The values must be encoded in "O" byte order and sorted in ascending order.
@param src       Pointer to the array of values.
@param bitstart  First bit to match, counted from the most significant bit of T (usually 0).
@param bitend    Last bit to match, counted from the most significant bit of T (7 for uint8_t, 15 for uint16_t, 31 for uint32_t, 63 for uint64_t). Must be at least bitstart and less than the width of T in bits.
@param first     First item of the search range (min value = 0).
@param last      Item past the last one of the search range (max value = nrows).
@param search    Values to search.
@param pos       Array of "count" items that receives, for each searched value, the item number when it is found or "last" when it is not.
@param count     Number of values to search.
@param prefetch  Whether to ask for the pages of a batch before reading them: BINSEARCH_PREFETCH_AUTO to ask only when a timed batch is slow enough to be reading from a device.
 */ \
static inline void col_find_many_sub_##O##_##T(const T *src, uint8_t bitstart, uint8_t bitend, uint64_t first, uint64_t last, const T *search, uint64_t *pos, uint64_t count, binsearch_prefetch_t prefetch) \
{ \
SUB_ITEM_VARS(T) \
FIND_MANY_VARS(T) \
FIND_MANY_LOOP(GET_COL_SUB_ITEM, ADDR_COL_SUB_ITEM, O, T) \
if (i < count) \
{ \
    uint64_t f = first; \
    uint64_t l = last; \
    pos[i] = col_find_first_sub_##O##_##T(src, bitstart, bitend, &f, &l, search[i]); \
} \
}

define_col_find_many_sub(be, uint8_t)
define_col_find_many_sub(be, uint16_t)
define_col_find_many_sub(be, uint32_t)
define_col_find_many_sub(be, uint64_t)
define_col_find_many_sub(le, uint8_t)
define_col_find_many_sub(le, uint16_t)
define_col_find_many_sub(le, uint32_t)
define_col_find_many_sub(le, uint64_t)

/**
 * Define a function that checks whether the next item matches the search value.
 *
 * @param O Byte order: be or le.
 * @param T Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t.
 */
#define define_col_has_next(O, T) \
/** Check whether the item after "pos" in a contiguous array of unsigned
integers of the same type matches the search value.
The values must be encoded in "O" byte order and sorted in ascending order.
Set "pos" to the item returned by the matching col_find_first function to walk
forward over the items that satisfy the search.
@param src       Pointer to the array of values.
@param pos       Pointer to the current item. Updated to the next item.
@param last      Item past the last one of the original search range (max value = nrows), not the value the find_first function wrote into its "last" argument.
@param search    Value to search.
@return true when the next item matches the search value, false otherwise.
 */ \
static inline bool col_has_next_##O##_##T(const T *src, uint64_t *pos, uint64_t last, T search) \
{ \
HAS_NEXT_START_BLOCK \
COL_HAS_END_BLOCK(O, T) \
}

define_col_has_next(be, uint8_t)
define_col_has_next(be, uint16_t)
define_col_has_next(be, uint32_t)
define_col_has_next(be, uint64_t)
define_col_has_next(le, uint8_t)
define_col_has_next(le, uint16_t)
define_col_has_next(le, uint32_t)
define_col_has_next(le, uint64_t)

/**
 * Define a function that checks whether the bits from bitstart to bitend of the
 * next item match the search value.
 *
 * @param O Byte order: be or le.
 * @param T Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t.
 */
#define define_col_has_next_sub(O, T) \
/** Check whether the bits from bitstart to bitend of the item after "pos", in a
contiguous array of unsigned integers of the same type, match the search value.
The values must be encoded in "O" byte order and sorted in ascending order.
Set "pos" to the item returned by the matching col_find_first_sub function to walk
forward over the items that satisfy the search.
@param src       Pointer to the array of values.
@param bitstart  First bit to match, counted from the most significant bit of T (usually 0).
@param bitend    Last bit to match, counted from the most significant bit of T (7 for uint8_t, 15 for uint16_t, 31 for uint32_t, 63 for uint64_t). Must be at least bitstart and less than the width of T in bits.
@param pos       Pointer to the current item. Updated to the next item.
@param last      Item past the last one of the original search range (max value = nrows), not the value the find_first function wrote into its "last" argument.
@param search    Value to search.
@return true when the bits of the next item match the search value, false otherwise.
 */ \
static inline bool col_has_next_sub_##O##_##T(const T *src, uint8_t bitstart, uint8_t bitend, uint64_t *pos, uint64_t last, T search) \
{ \
HAS_NEXT_START_BLOCK \
SUB_ITEM_VARS(T) \
COL_HAS_SUB_END_BLOCK(O, T) \
}

define_col_has_next_sub(be, uint8_t)
define_col_has_next_sub(be, uint16_t)
define_col_has_next_sub(be, uint32_t)
define_col_has_next_sub(be, uint64_t)
define_col_has_next_sub(le, uint8_t)
define_col_has_next_sub(le, uint16_t)
define_col_has_next_sub(le, uint32_t)
define_col_has_next_sub(le, uint64_t)

/**
 * Define a function that checks whether the previous item matches the search value.
 *
 * @param O Byte order: be or le.
 * @param T Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t.
 */
#define define_col_has_prev(O, T) \
/** Check whether the item before "pos" in a contiguous array of unsigned
integers of the same type matches the search value.
The values must be encoded in "O" byte order and sorted in ascending order.
Set "pos" to the item returned by the matching col_find_last function to walk
backward over the items that satisfy the search.
@param src       Pointer to the array of values.
@param first     First item of the original search range (min value = 0), not the value the find_last function wrote into its "first" argument.
@param pos       Pointer to the current item. Updated to the previous item.
@param search    Value to search.
@return true when the previous item matches the search value, false otherwise.
 */ \
static inline bool col_has_prev_##O##_##T(const T *src, uint64_t first, uint64_t *pos, T search) \
{ \
HAS_PREV_START_BLOCK \
COL_HAS_END_BLOCK(O, T) \
}

define_col_has_prev(be, uint8_t)
define_col_has_prev(be, uint16_t)
define_col_has_prev(be, uint32_t)
define_col_has_prev(be, uint64_t)
define_col_has_prev(le, uint8_t)
define_col_has_prev(le, uint16_t)
define_col_has_prev(le, uint32_t)
define_col_has_prev(le, uint64_t)

/**
 * Define a function that checks whether the bits from bitstart to bitend of the
 * previous item match the search value.
 *
 * @param O Byte order: be or le.
 * @param T Unsigned integer type, one of: uint8_t, uint16_t, uint32_t, uint64_t.
 */
#define define_col_has_prev_sub(O, T) \
/** Check whether the bits from bitstart to bitend of the item before "pos", in
a contiguous array of unsigned integers of the same type, match the search
value.
The values must be encoded in "O" byte order and sorted in ascending order.
Set "pos" to the item returned by the matching col_find_last_sub function to walk
backward over the items that satisfy the search.
@param src       Pointer to the array of values.
@param bitstart  First bit to match, counted from the most significant bit of T (usually 0).
@param bitend    Last bit to match, counted from the most significant bit of T (7 for uint8_t, 15 for uint16_t, 31 for uint32_t, 63 for uint64_t). Must be at least bitstart and less than the width of T in bits.
@param first     First item of the original search range (min value = 0), not the value the find_last function wrote into its "first" argument.
@param pos       Pointer to the current item. Updated to the previous item.
@param search    Value to search.
@return true when the bits of the previous item match the search value, false otherwise.
 */ \
static inline bool col_has_prev_sub_##O##_##T(const T *src, uint8_t bitstart, uint8_t bitend, uint64_t first, uint64_t *pos, T search) \
{ \
HAS_PREV_START_BLOCK \
SUB_ITEM_VARS(T) \
COL_HAS_SUB_END_BLOCK(O, T) \
}

define_col_has_prev_sub(be, uint8_t)
define_col_has_prev_sub(be, uint16_t)
define_col_has_prev_sub(be, uint32_t)
define_col_has_prev_sub(be, uint64_t)
define_col_has_prev_sub(le, uint8_t)
define_col_has_prev_sub(le, uint16_t)
define_col_has_prev_sub(le, uint32_t)
define_col_has_prev_sub(le, uint64_t)

// --- FILE ---

#define BINSEARCH_MIN_FILE_SIZE 28 //!< Minimum file size required to hold a format header

/**
 * Set the data block of the memory mapped file to empty.
 *
 * The column description is cleared as well, so that a caller that ignores the
 * number of rows cannot read from an offset left behind by a rejected header,
 * and so that a rejected header does not leave the columns the caller declared
 * next to the cleared results of parsing, where they read as if the file had
 * described them.
 *
 * @param mf Descriptor of the memory mapped file.
 */
static inline void clear_data_block(mmfile_t *mf)
{
    mf->doffset = 0;
    mf->dlength = 0;
    mf->nrows = 0;
    mf->ncols = 0;
    memset(mf->ctbytes, 0, sizeof(mf->ctbytes));
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
    for (i = 0; i < mf->ncols; i++)
    {
        b += mf->ctbytes[i];
    }
    if (b == 0)
    {
        // The file declares no column, or every column type size is zero, so no
        // column layout can be computed. The offsets are left empty, as there is
        // no column to read, while doffset and dlength keep describing the data
        // block of the file.
        return;
    }
    mf->index[0] = mf->doffset;
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
 * The border field reports the byte order of the values: every supported
 * format stores them little-endian, so it is always BINSEARCH_ORDER_LE.
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
    // Every format the header is sniffed for stores its values little-endian.
    // Raw content carries no header to declare one, so it gets the same value:
    // a caller that knows better overwrites the field after the call.
    mf->border = (uint8_t)BINSEARCH_ORDER_LE;
    mf->prefetch = BINSEARCH_PREFETCH_AUTO;
    memset(mf->index, 0, sizeof(mf->index));
    struct stat statbuf;
    int err = 0;
    mf->fd = open(file, BINSEARCH_OPEN_FLAGS);
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
    if (!S_ISREG(statbuf.st_mode))
    {
        // Only a regular file has a size that describes the bytes that can be
        // read from it: the size of a directory, a FIFO or a device does not,
        // and mapping one would report a data block that cannot be read.
        close(mf->fd);
        mf->fd = -1;
        errno = EINVAL;
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
 * Tell the kernel that the mapping is read in a scattered order, so that it
 * does not read ahead the pages that surround the ones a search touches.
 *
 * It is a hint: a caller that scans the data block as well as searching it is
 * better off without it.
 *
 * @param mf Descriptor of the memory mapped file.
 *
 * @return 0 on success, -1 on failure or when the platform provides no way to
 *         give the hint.
 */
static inline int binsearch_advise_random(const mmfile_t *mf)
{
    if ((mf->src == (uint8_t *)MAP_FAILED) || (mf->size == 0))
    {
        errno = EINVAL;
        return -1;
    }
    return ((BINSEARCH_ADVISE(mf->src, (size_t)mf->size) == 0) ? 0 : -1);
}

/**
 * Take the address of a mapping and leave MAP_FAILED behind, as one step.
 *
 * Two threads unmapping the same descriptor is a mistake, but it is the kind
 * that has to fail safely: a plain read followed by a write lets both of them
 * see an address, unmap it twice and close the descriptor twice, and the second
 * close lands on whatever descriptor number the process has reused in between.
 * Exchanging the address means only one of them gets it.
 *
 * The fallback is a plain read and write, on a compiler that offers no atomic
 * exchange. It is enough for a single-threaded caller, which is all such a
 * build can promise.
 *
 * @param src Address of the field holding the address of the mapping.
 *
 * @return The address the field held, or MAP_FAILED when it held no mapping.
 *
 * @private
 */
static inline uint8_t *binsearch_atomic_take(uint8_t **src)
{
#if defined(__GNUC__) || defined(__clang__)
    return __atomic_exchange_n(src, (uint8_t *)MAP_FAILED, __ATOMIC_ACQ_REL);
#else
    uint8_t *old = *src;
    *src = (uint8_t *)MAP_FAILED;
    return old;
#endif
}

/**
 * Unmap and close the memory mapped file.
 *
 * On success the descriptor is reset to the state of a file that was never
 * mapped (src = MAP_FAILED, fd = -1, size = 0), so that a second call reports
 * an error instead of unmapping an address and closing a descriptor that have
 * been reused since. The address is taken with an exchange, so two calls that
 * overlap cannot both reach the unmapping.
 *
 * The file descriptor is left untouched when the unmapping fails, because a
 * descriptor that does not belong to a valid mapping must not be closed, and
 * the address is put back so that the caller can act on the failure.
 *
 * @param mf Descriptor of the memory mapped file.
 *
 * @return 0 on success, -1 on failure with errno set.
 */
static inline int munmap_binfile(mmfile_t *mf)
{
    uint8_t *src = binsearch_atomic_take(&mf->src);
    if (src == (uint8_t *)MAP_FAILED)
    {
        errno = EINVAL;
        return -1;
    }
    if (munmap(src, (size_t)mf->size) != 0)
    {
        mf->src = src; // the mapping is still there, and so is its descriptor
        return -1;
    }
    const int fd = mf->fd;
    mf->fd = -1;
    mf->size = 0;
    clear_data_block(mf);
    return close(fd);
}

//!< \cond

// The code generation macros are an implementation detail: they are undefined
// here so that they do not leak into the translation units that include this
// header. The documented ones (MAXCOLS, get_address, get_src_offset and the
// order_* family) are kept.

#undef define_bytes_to
#undef define_get_src_offset
#undef define_find_first
#undef define_find_first_sub
#undef define_find_last
#undef define_find_last_sub
#undef define_find_range
#undef define_find_range_sub
#undef define_find_many
#undef define_find_many_sub
#undef define_has_next
#undef define_has_next_sub
#undef define_has_prev
#undef define_has_prev_sub
#undef define_col_find_first
#undef define_col_find_first_sub
#undef define_col_find_last
#undef define_col_find_last_sub
#undef define_col_find_range
#undef define_col_find_range_sub
#undef define_col_find_many
#undef define_col_find_many_sub
#undef define_col_has_next
#undef define_col_has_next_sub
#undef define_col_has_prev
#undef define_col_has_prev_sub

#undef GET_ROW_ITEM
#undef GET_COL_ITEM
#undef GET_ROW_SUB_ITEM
#undef GET_COL_SUB_ITEM
#undef FIND_VARS
#undef FIND_RANGE_VARS
#undef FIND_RANGE_BODY
#undef FIND_MANY_VARS
#undef FIND_MANY_BLOCK
#undef FIND_MANY_LOOP
#undef FIND_MANY_WILLNEED
#undef FIND_MANY_PREFETCH_VARS
#undef FIND_MANY_PREFETCH_STEP
#undef FIND_MANY_PREFETCH_START
#undef FIND_MANY_PROBE_BEGIN
#undef FIND_MANY_PROBE_END
#undef ADDR_ROW_ITEM
#undef ADDR_COL_ITEM
#undef ADDR_ROW_SUB_ITEM
#undef ADDR_COL_SUB_ITEM
#undef FIND_FIRST_LOOP
#undef FIND_LAST_LOOP
#undef FIND_NEW_LAST
#undef FIND_RETURN_NOT_FOUND
#undef FIND_FIRST_TAIL
#undef FIND_LAST_TAIL
#undef SUB_ITEM_VARS
#undef HAS_NEXT_START_BLOCK
#undef HAS_PREV_START_BLOCK
#undef GET_POS_BLOCK
#undef HAS_END_BLOCK
#undef HAS_SUB_END_BLOCK
#undef COL_HAS_END_BLOCK
#undef COL_HAS_SUB_END_BLOCK

//!< \endcond

#endif  // VARIANTKEY_BINSEARCH_H
