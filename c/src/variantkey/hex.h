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
 * @brief Utility functions to manipulate strings.
 *
 * Collection of utility functions to manipulate strings.
 */

#ifndef VARIANTKEY_HEX_H
#define VARIANTKEY_HEX_H

#include <inttypes.h>
#include <stdio.h>

/** @brief Returns uint64_t hexadecimal string (16 characters).
 *
 * @param n     Number to parse
 * @param str   String buffer to be returned (it must be sized 17 bytes at least).
 *
 * @return      Upon successful return, these function returns the number of characters processed
 *              (excluding the null byte used to end output to strings).
 *              If the buffer size is not sufficient, then the return value is the number of characters required for
 *              buffer string, including the terminating null byte.
 */
static inline size_t hex_uint64_t(uint64_t n, char *str)
{
    return sprintf(str, "%016" PRIx64, n);
}

/** @brief Parses a 16 chars hexadecimal string and returns the code.
 *
 * @param s    Hexadecimal string to parse (it must contain 16 hexadecimal characters).
 *
 * @return uint64_t unsigned integer number.
 */
static inline uint64_t parse_hex_uint64_t(const char *s)
{
    uint64_t v = 0;
    uint8_t b = 0;
    size_t i = 0;
    for (i = 0; i < 16; i++)
    {
        b = s[i];
        if (b >= 'a')
        {
            b -= ('a' - 10); // a-f
        }
        else
        {
            if (b >= 'A')
            {
                b -= ('A' - 10); // A-F
            }
            else
            {
                b -= '0'; // 0-9
            }
        }
        v = ((v << 4) | b);
    }
    return v;
}

#endif  // VARIANTKEY_HEX_H
