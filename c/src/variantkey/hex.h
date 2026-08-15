// VariantKey
//
// hex.h
//
// @category   Libraries
// @author     Nicola Asuni <info@tecnick.com>
// @link       https://github.com/tecnickcom/variantkey
// @license    MIT [LICENSE](https://raw.githubusercontent.com/tecnickcom/variantkey/main/LICENSE)

/**
 * @file hex.h
 * @brief Conversion between uint64_t values and 16 character hexadecimal strings.
 */

#ifndef VARIANTKEY_HEX_H
#define VARIANTKEY_HEX_H

#include <inttypes.h>
#include <stdint.h>
#include <stddef.h>

#define HEX_UINT64_LEN 16 //!< Number of hexadecimal characters of a uint64_t, excluding the terminating null byte

/**
 * @brief Writes a uint64_t as a zero-padded lowercase hexadecimal string of 16 characters.
 *
 * @param n     Number to convert.
 * @param str   String buffer to be returned (it must be sized 17 bytes at least).
 *
 * @return      The number of characters written, excluding the terminating null byte.
 */
static inline size_t hex_uint64_t(uint64_t n, char *str)
{
    static const char hexmap[] = "0123456789abcdef";
    size_t i = HEX_UINT64_LEN;
    while (i--)
    {
        str[i] = hexmap[(n & 0xF)];
        n >>= 4;
    }
    str[HEX_UINT64_LEN] = 0;
    return HEX_UINT64_LEN;
}

/**
 * @brief Parses a 16 character hexadecimal string into a uint64_t.
 *
 * The letters can be lowercase or uppercase. Any other character produces an
 * unspecified value.
 *
 * @param s    Hexadecimal string to parse (it must contain 16 hexadecimal characters).
 *
 * @return The parsed number.
 */
static inline uint64_t parse_hex_uint64_t(const char *s)
{
    uint64_t v = 0;
    size_t i = 0;
    for (i = 0; i < HEX_UINT64_LEN; i++)
    {
        uint8_t b = (uint8_t)s[i];
        // Branchless nibble decode.
        // '0'-'9' have bit 6 clear so they contribute (b & 0xF) directly, while
        // 'A'-'F' and 'a'-'f' have bit 6 set and need the extra 9:
        //   '0' -> 0 + 0 = 0    '9' -> 9 + 0 = 9
        //   'A' -> 1 + 9 = 10   'F' -> 6 + 9 = 15
        //   'a' -> 1 + 9 = 10   'f' -> 6 + 9 = 15
        v = ((v << 4) | (uint64_t)((b & 0xF) + ((b >> 6) * 9)));
    }
    return v;
}

#endif  // VARIANTKEY_HEX_H
