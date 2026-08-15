// VariantKey
//
// esid.h
//
// @category   Libraries
// @author     Nicola Asuni <info@tecnick.com>
// @link       https://github.com/tecnickcom/variantkey
// @license    MIT [LICENSE](https://raw.githubusercontent.com/tecnickcom/variantkey/main/LICENSE)

/**
 * @file esid.h
 * @brief Encoding and decoding of string IDs as 64 bit unsigned integers.
 *
 * Strings of up to ESID_MAXLEN characters are encoded reversibly at 6 bit per
 * character. Longer strings made of a character section, a separator and a
 * numerical section are encoded reversibly by encode_string_num_id. Any other
 * string can be reduced to a non-reversible hash by hash_string_id.
 */

#ifndef VARIANTKEY_ESID_H
#define VARIANTKEY_ESID_H

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define ESID_MAXLEN   10 //!< Maximum number of characters that can be encoded
#define ESID_SHIFT    32 //!< Number used to translate ASCII character values
#define ESID_SHIFTPOS 60 //!< Encoded string ID LEN LSB position from LSB [ ----0000 00111111 22222233 33334444 44555555 66666677 77778888 88999999 ]
#define ESID_CHARBIT  6  //!< Number of bit used to encode a char
#define ESID_NUMPOS   27 //!< Number of bit used to encode a number in the string_num encoding
#define ESID_MAXPAD   7  //!< Max number of padding zero digits
#define ESID_MAXSTRLEN 23 //!< Size in bytes of the buffer required to decode any encoded string ID, including the terminating null byte

/**
 * @brief Encodes a single character into a 6 bit value.
 *
 * Lowercase letters are folded to uppercase and any character below '!' is
 * encoded as '_'.
 *
 * @param c    Character to encode. It must be an ASCII character from '!' to 'z'.
 *
 * @return The encoded character.
 */
static inline uint64_t esid_encode_char(int c)
{
    // 256 byte lookup table indexed by (uint8_t)c, so the result does not depend
    // on the signedness of plain char.
    static const uint8_t map[256] =
    {
        63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
        63,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
        32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
        32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
        63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
        63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
        63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
        63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
    };
    return (uint64_t)map[(uint8_t)c];
}

/**
 * @brief Decodes the character stored at the given bit position of an encoded string ID.
 *
 * @param esid  Encoded string ID.
 * @param pos   Bit position of the character.
 *
 * @return The decoded character.
 */
static inline char esid_decode_char(uint64_t esid, size_t pos)
{
    return (char)(((esid >> pos) & 0x3f) + ESID_SHIFT); // 0x3f hex = 63 dec = 00111111 bin
}

/**
 * @brief Encodes up to ESID_MAXLEN characters of a string into a 64 bit unsigned integer.
 *
 * Characters beyond the first ESID_MAXLEN from start are ignored.
 *
 * @param str    String to encode. Supports ASCII characters from '!' to 'z'.
 * @param size   Length of the string, excluding the terminating null byte.
 * @param start  First character to encode, counting from 0. To encode the last ESID_MAXLEN characters, set this to (size - ESID_MAXLEN).
 *
 * @return The encoded string ID, or 0 if start is greater than size.
 */
static inline uint64_t encode_string_id(const char *str, size_t size, size_t start)
{
    if (start > size)
    {
        return 0;
    }
    size -= start;
    if (size > ESID_MAXLEN)
    {
        size = ESID_MAXLEN;
    }
    str += start;
    const char *pos = (str + size);
    // NOTE: the cast is required because size_t is 32 bit wide on ILP32
    // platforms, where shifting it by ESID_SHIFTPOS would be undefined.
    uint64_t h = ((uint64_t)size << ESID_SHIFTPOS);
    switch (size)
    {
    case 10:
        h |= esid_encode_char(*(--pos));
    // fall through
    case 9:
        h |= esid_encode_char(*(--pos)) << (size_t)(ESID_CHARBIT * 1);
    // fall through
    case 8:
        h |= esid_encode_char(*(--pos)) << (size_t)(ESID_CHARBIT * 2);
    // fall through
    case 7:
        h |= esid_encode_char(*(--pos)) << (size_t)(ESID_CHARBIT * 3);
    // fall through
    case 6:
        h |= esid_encode_char(*(--pos)) << (size_t)(ESID_CHARBIT * 4);
    // fall through
    case 5:
        h |= esid_encode_char(*(--pos)) << (size_t)(ESID_CHARBIT * 5);
    // fall through
    case 4:
        h |= esid_encode_char(*(--pos)) << (size_t)(ESID_CHARBIT * 6);
    // fall through
    case 3:
        h |= esid_encode_char(*(--pos)) << (size_t)(ESID_CHARBIT * 7);
    // fall through
    case 2:
        h |= esid_encode_char(*(--pos)) << (size_t)(ESID_CHARBIT * 8);
    // fall through
    case 1:
        h |= esid_encode_char(*(--pos)) << (size_t)(ESID_CHARBIT * 9);
    default:
        break;
    }
    return h;
}

/**
 * @brief Encodes a string made of a character section, a separator and a numerical section.
 *
 * For example "ABCDE:0001234". Up to 5 characters, up to ESID_MAXPAD leading
 * zeros and a number below 2^27 are encoded. Strings of ESID_MAXLEN characters
 * or less are passed to encode_string_id instead. When no separator is found
 * within size the numerical section is left empty.
 *
 * @param str    String to encode. It must be null terminated and supports ASCII characters from '!' to 'z'.
 * @param size   Length of the string, excluding the terminating null byte.
 * @param sep    Separator character between the two sections.
 *
 * @return The encoded string ID.
 */
static inline uint64_t encode_string_num_id(const char *str, size_t size, char sep)
{
    if (size <= ESID_MAXLEN)
    {
        return encode_string_id(str, size, 0);
    }
    uint64_t h = 0;
    uint32_t num = 0;
    uint8_t nchr = 0, npad = 0;
    uint8_t bitpos = ESID_SHIFTPOS;
    int c = 0;
    while ((c = (unsigned char) *str++) && (size--))
    {
        if (c == sep)
        {
            break;
        }
        if (nchr < 5)
        {
            bitpos -= ESID_CHARBIT;
            h |= (esid_encode_char(c) << bitpos);
            nchr++;
        }
    }
    h |= ((uint64_t)(nchr + ESID_MAXLEN) << ESID_SHIFTPOS); // 4 bit for string length
    if (c == 0)
    {
        return h; // the string ended before the separator: there is no numerical section
    }
    while (((c = (unsigned char) *str++) == '0') && (npad < ESID_MAXPAD) && (size--))
    {
        npad++;
    }
    h |= (npad << ESID_NUMPOS); // 3 bit for 0 padding length
    while ((c >= '0') && (c <= '9') && (size--))
    {
        num = ((num * 10) + (c - '0'));
        c = (unsigned char) *str++;
    }
    h |= ((uint64_t)num & 0x7FFFFFF); // 27 bit for number
    return h;
}

/**
 * @brief Decodes the character section of an encoded string ID.
 *
 * The string is always returned in uppercase.
 *
 * @param size   Number of characters to decode, from 0 to ESID_MAXLEN.
 * @param esid   Encoded string ID.
 * @param str    String buffer to be returned (it must be sized ESID_MAXLEN + 1 bytes at least).
 *
 * @return The number of characters written, excluding the terminating null byte.
 */
static inline size_t esid_decode_string_id(size_t size, uint64_t esid, char *str)
{
    switch (size)
    {
    case 10:
        str[9] = esid_decode_char(esid, (size_t)0);
    // fall through
    case 9:
        str[8] = esid_decode_char(esid, (size_t)(ESID_CHARBIT * 1));
    // fall through
    case 8:
        str[7] = esid_decode_char(esid, (size_t)(ESID_CHARBIT * 2));
    // fall through
    case 7:
        str[6] = esid_decode_char(esid, (size_t)(ESID_CHARBIT * 3));
    // fall through
    case 6:
        str[5] = esid_decode_char(esid, (size_t)(ESID_CHARBIT * 4));
    // fall through
    case 5:
        str[4] = esid_decode_char(esid, (size_t)(ESID_CHARBIT * 5));
    // fall through
    case 4:
        str[3] = esid_decode_char(esid, (size_t)(ESID_CHARBIT * 6));
    // fall through
    case 3:
        str[2] = esid_decode_char(esid, (size_t)(ESID_CHARBIT * 7));
    // fall through
    case 2:
        str[1] = esid_decode_char(esid, (size_t)(ESID_CHARBIT * 8));
    // fall through
    case 1:
        str[0] = esid_decode_char(esid, (size_t)(ESID_CHARBIT * 9));
    default:
        break;
    }
    str[size] = 0;
    return size;
}

/**
 * @brief Decodes an encoded string ID that carries a numerical section.
 *
 * This is the reverse of encode_string_num_id. The string is always returned in
 * uppercase and the separator is always a colon.
 *
 * @param size   Number of characters of the character section.
 * @param esid   Encoded string ID.
 * @param str    String buffer to be returned (it must be sized ESID_MAXSTRLEN bytes at least).
 *
 * @return The number of characters written, excluding the terminating null byte.
 */
static inline size_t esid_decode_string_num_id(size_t size, uint64_t esid, char *str)
{
    size = esid_decode_string_id(size, esid, str);
    str[size++] = ':';
    uint8_t npad = (uint8_t)((esid >> ESID_NUMPOS) & ESID_MAXPAD);
    while (npad--)
    {
        str[size++] = '0';
    }
    uint64_t num = (esid & 0x7FFFFFF);
    if (num > 0)
    {
        size += (size_t)snprintf((str + size), (ESID_MAXSTRLEN - size), "%" PRIu64, num);
    }
    str[size] = 0;
    return size;
}

/**
 * @brief Decodes an encoded string ID.
 *
 * This is the reverse of encode_string_id and encode_string_num_id: the length
 * field of the code selects which of the two forms is decoded. The string is
 * always returned in uppercase.
 *
 * @param esid   Encoded string ID.
 * @param str    String buffer to be returned (it must be sized ESID_MAXSTRLEN bytes at least).
 *
 * @return The number of characters written, excluding the terminating null byte.
 */
static inline size_t decode_string_id(uint64_t esid, char *str)
{
    size_t size = (esid >> ESID_SHIFTPOS);
    if (size > ESID_MAXLEN)
    {
        return esid_decode_string_num_id((size - ESID_MAXLEN), esid, str);
    }
    return esid_decode_string_id(size, esid, str);
}

/**
 * @brief Mixes a 64 bit key into a 64 bit hash with the MurmurHash3 round function.
 *
 * @param k    Key to mix.
 * @param h    Hash to mix the key into.
 *
 * @return The mixed hash value.
 */
static inline uint64_t muxhash64(uint64_t k, uint64_t h)
{
    k *= 0x87c37b91114253d5;
    k = (k >> 33) | (k << 31);
    k *= 0x4cf5ad432745937f;
    h ^= k;
    h = (h >> 37) | (h << 27);
    return ((h * 5) + 0x52dce729);
}

/**
 * @brief Hashes a string into a non-reversible 64 bit string ID.
 *
 * The most significant bit of the result is always set to mark the hash mode.
 * The 8-byte blocks are read in the byte order of the host, so the result is
 * endianness dependent.
 *
 * @param str    String to hash.
 * @param size   Length of the string, excluding the terminating null byte.
 *
 * @return The hashed string ID.
 */
static inline uint64_t hash_string_id(const char *str, size_t size)
{
    // NOTE: the 8-byte blocks are read in the byte order of the host,
    // so the returned value is endianness dependent.
    // The memcpy of a compile-time constant size is subject to neither the
    // alignment nor the strict-aliasing rules of a pointer cast.
    const uint8_t *pos = (const uint8_t *)str;
    const uint8_t *end = (pos + ((size / 8) * 8));
    uint64_t h = 0;
    uint64_t b = 0;
    while (pos < end)
    {
        memcpy(&b, pos, sizeof(b));
        h = muxhash64(b, h);
        pos += sizeof(b);
    }
    const uint8_t *tail = pos;
    uint64_t v = 0;
    switch (size & 7)
    {
    case 7:
        v ^= (uint64_t)tail[6] << (8 * 6);
    // fall through
    case 6:
        v ^= (uint64_t)tail[5] << (8 * 5);
    // fall through
    case 5:
        v ^= (uint64_t)tail[4] << (8 * 4);
    // fall through
    case 4:
        v ^= (uint64_t)tail[3] << (8 * 3);
    // fall through
    case 3:
        v ^= (uint64_t)tail[2] << (8 * 2);
    // fall through
    case 2:
        v ^= (uint64_t)tail[1] << (8 * 1);
    // fall through
    case 1:
        v ^= (uint64_t)tail[0];
        break;
    default:
        break;
    }
    if (v > 0)
    {
        h = muxhash64(v, h);
    }
    // MurmurHash3 finalization mix - force all bits of a hash block to avalanche
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccd;
    h ^= h >> 33;
    h *= 0xc4ceb9fe1a85ec53;
    h ^= h >> 33;
    return (h | 0x8000000000000000); // set the first bit to indicate HASH mode
}

#endif  // VARIANTKEY_ESID_H
