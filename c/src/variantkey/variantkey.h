// VariantKey
//
// variantkey.h
//
// @category   Libraries
// @author     Nicola Asuni <info@tecnick.com>
// @link       https://github.com/tecnickcom/variantkey
// @license    MIT [LICENSE](https://raw.githubusercontent.com/tecnickcom/variantkey/main/LICENSE)

/**
 * @file variantkey.h
 * @brief Encoding and decoding of the 64 bit VariantKey.
 *
 * A VariantKey packs a chromosome in 5 bit, a position in 28 bit and a
 * reference and alternate allele pair in 31 bit. Keys sort by chromosome and
 * position, and are fully reversible for allele pairs of up to
 * VKMAX_REFALT_LEN bases made only of A, C, G and T.
 */

#ifndef VARIANTKEY_VARIANTKEY_H
#define VARIANTKEY_VARIANTKEY_H

#include <inttypes.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include "hex.h"

#define VKMASK_CHROM    0xF800000000000000  //!< VariantKey binary mask for CHROM     [ 11111000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 ]
#define VKMASK_POS      0x07FFFFFF80000000  //!< VariantKey binary mask for POS       [ 00000111 11111111 11111111 11111111 10000000 00000000 00000000 00000000 ]
#define VKMASK_CHROMPOS 0xFFFFFFFF80000000  //!< VariantKey binary mask for CHROM+POS [ 11111111 11111111 11111111 11111111 10000000 00000000 00000000 00000000 ]
#define VKMASK_REFALT   0x000000007FFFFFFF  //!< VariantKey binary mask for REF+ALT   [ 00000000 00000000 00000000 00000000 01111111 11111111 11111111 11111111 ]
#define VKSHIFT_CHROM   59 //!< CHROM LSB position from the VariantKey LSB
#define VKSHIFT_POS     31 //!< POS LSB position from the VariantKey LSB
#define MAXUINT32       0xFFFFFFFF //!< Maximum value for uint32_t
#define VKMAX_REFALT_LEN 11 //!< Maximum total number of REF+ALT bases of a reversible REF+ALT encoding
#define VKMAX_ALLELE_LEN 10 //!< Maximum number of bases of a single allele of a reversible REF+ALT encoding

/**
 * @brief The numerically encoded VariantKey components.
 */
typedef struct variantkey_t
{
    uint8_t chrom;   //!< Chromosome encoded number (only the LSB 5 bit are used)
    uint32_t pos;    //!< Reference position, with the first base having position 0 (only the LSB 28 bit are used)
    uint32_t refalt; //!< Code for Reference and Alternate allele (only the LSB 31 bits are used)
} variantkey_t;

/**
 * @brief The minimum and maximum VariantKey values of a range search.
 */
typedef struct vkrange_t
{
    uint64_t min; //!< Minimum VariantKey value for any given REF+ALT encoding
    uint64_t max; //!< Maximum VariantKey value for any given REF+ALT encoding
} vkrange_t;

/**
 * @brief Encodes a chromosome string made only of digits.
 *
 * The value is accumulated in 8 bit, so an input above 255 wraps around.
 *
 * @param chrom  Chromosome identifier, no white-space permitted.
 * @param size   Length of the chrom string, excluding the terminating null byte.
 *
 * @return CHROM code, or 0 if a non-digit character is found.
 */
static inline uint8_t encode_numeric_chrom(const char *chrom, size_t size)
{
    uint8_t v = 0;
    for (size_t i = 0; i < size; ++i)
    {
        unsigned char c = (unsigned char)chrom[i];
        if (c < '0' || c > '9')
        {
            return 0; // NA: a character that is not a number was found.
        }
        v = (uint8_t)((v * 10) + (c - '0'));
    }
    return v;
}


/**
 * @brief Checks if the chromosome string starts with a case-insensitive "chr" prefix.
 *
 * The prefix is only recognised when it is followed by at least one character.
 *
 * @param chrom  Chromosome identifier, no white-space permitted.
 * @param size   Length of the chrom string, excluding the terminating null byte.
 *
 * @return 1 if the prefix is present, 0 otherwise.
 */
static inline int has_chrom_chr_prefix(const char *chrom, size_t size)
{
    if (size > 3)
    {
        // case-insensitive comparison for chr or CHR prefix
        uint32_t v = ((uint8_t)chrom[0] | 0x20) << 16 | ((uint8_t)chrom[1] | 0x20) << 8 | ((uint8_t)chrom[2] | 0x20);
        // 'c' = 0x63, 'h' = 0x68, 'r' = 0x72
        return (v == 0x636872);
    }
    return 0;
}

/**
 * @brief Encodes a chromosome identifier into a numerical code.
 *
 * A leading case-insensitive "chr" prefix is removed. Digits are encoded by
 * their numerical value, X as 23, Y as 24, M or MT as 25.
 *
 * @param chrom  Chromosome identifier, no white-space permitted.
 * @param size   Length of the chrom string, excluding the terminating null byte.
 *
 * @return CHROM code, or 0 for an empty or unrecognised identifier.
 */
static inline uint8_t encode_chrom(const char *chrom, size_t size)
{
    // X = 23; Y = 24; M = 25; any other letter is mapped to 0:
    // *INDENT-OFF*
    static const uint8_t onecharmap[] =
    {
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        /*                                                   M                                           X   Y */
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 25,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 23, 24,  0,  0,  0,  0,  0,  0,
        /*                                                   m                                           x   y */
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 25,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 23, 24,  0,  0,  0,  0,  0,  0,
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    };
    // *INDENT-ON*
    if (has_chrom_chr_prefix(chrom, size))
    {
        // remove "chr" or "CHR" prefix
        chrom += 3;
        size -= 3;
    }
    if (size == 0)
    {
        return 0;
    }
    if ((chrom[0] <= '9') && (chrom[0] >= '0'))
    {
        return encode_numeric_chrom(chrom, size);
    }
    if ((size == 1) || ((size == 2) && (((uint8_t)chrom[1] | 0x20) == 't')))
    {
        return onecharmap[((uint8_t)chrom[0])];
    }
    return 0; // NA
}

/**
 * @brief Decodes a chromosome numerical code into its string representation.
 *
 * Codes 1 to 22 decode to their digits, 23 to X, 24 to Y, 25 to MT.
 * Any other code decodes to NA.
 *
 * @param code   CHROM code.
 * @param chrom  CHROM string buffer to be returned (it must be sized 3 bytes at least).
 *
 * @return The number of characters written, excluding the terminating null byte.
 */
static inline size_t decode_chrom(uint8_t code, char *chrom)
{
    // Fast path for numeric chromosomes 1-22
    if ((code >= 1) && (code <= 22))
    {
        if (code < 10)
        {
            chrom[0] = (char)('0' + code);
            chrom[1] = '\0';
            return 1;
        }
        chrom[0] = (char)('0' + (code / 10));
        chrom[1] = (char)('0' + (code % 10));
        chrom[2] = '\0';
        return 2;
    }
    // X=23
    if (code == 23)
    {
        chrom[0] = 'X';
        chrom[1] = '\0';
        return 1;
    }

    // Y=24
    if (code == 24)
    {
        chrom[0] = 'Y';
        chrom[1] = '\0';
        return 1;
    }
    // MT=25
    if (code == 25)
    {
        chrom[0] = 'M';
        chrom[1] = 'T';
        chrom[2] = '\0';
        return 2;
    }
    // Invalid code 'NA'
    chrom[0] = 'N';
    chrom[1] = 'A';
    chrom[2] = '\0';
    return 2;
}

/**
 * @brief Encodes a nucleotide letter into a 2 bit code.
 *
 * The letters are case-insensitive.
 *
 * @param c  Nucleotide character to encode.
 *
 * @return A=0, C=1, G=2, T=3, or 4 for any other character.
 */
static inline uint32_t encode_base(const uint8_t c)
{
    // 256 byte lookup table indexed by the character value.
    static const uint8_t map[256] =
    {
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        /* A     C           G                                      T */
        4, 0, 4, 1, 4, 4, 4, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        /* a     c           g                                      t */
        4, 0, 4, 1, 4, 4, 4, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    };
    return (uint32_t)map[c];
}

/**
 * @brief Encodes an allele into 2 bit per base, from the given bit position downwards.
 *
 * The hash and the bit position are updated in place.
 *
 * @param h        Pointer to the 32 bit code to update.
 * @param bitpos   Pointer to the bit position of the next base.
 * @param str      Allele string to encode.
 * @param size     Length of the allele string, excluding the terminating null byte.
 *
 * @return 0 on success, or -1 if a character other than A, C, G or T is found.
 */
static inline int encode_allele(uint32_t *h, uint8_t *bitpos, const char *str, size_t size)
{
    while (size--)
    {
        uint32_t v = encode_base(*str++);
        if (v > 3)
        {
            return -1;
        }
        *bitpos -= 2;
        *h |= (v << *bitpos);
    }
    return 0;
}

/**
 * @brief Encodes a REF+ALT pair with the reversible scheme.
 *
 * The code carries the two allele lengths in 4 bit each, followed by 2 bit per
 * base. The caller must check that each allele is VKMAX_ALLELE_LEN bases or
 * less and that the total length is VKMAX_REFALT_LEN or less.
 *
 * @param ref      Reference allele string.
 * @param sizeref  Length of the ref string, excluding the terminating null byte.
 * @param alt      Alternate allele string.
 * @param sizealt  Length of the alt string, excluding the terminating null byte.
 *
 * @return The 32 bit REF+ALT code, or MAXUINT32 if an allele contains a character other than A, C, G or T.
 */
static inline uint32_t encode_refalt_rev(const char *ref, size_t sizeref, const char *alt, size_t sizealt)
{
    // [******** ******** ******** ******** *RRRRAAA A1122334 45566778 8990011*]
    // RRRR: length of REF ; AAAA: length of ALT
    uint32_t h = ((uint32_t)(sizeref) << 27) |((uint32_t)(sizealt) << 23);
    uint8_t bitpos = 23;
    if ((encode_allele(&h, &bitpos, ref, sizeref) < 0) || (encode_allele(&h, &bitpos, alt, sizealt) < 0))
    {
        return MAXUINT32; // error code
    }
    return h;
}

/**
 * @brief Mixes a 32 bit key into a 32 bit hash with the MurmurHash3 round function.
 *
 * @param k  Key to mix.
 * @param h  Hash to mix the key into.
 *
 * @return The mixed hash value.
 */
static inline uint32_t muxhash(uint32_t k, uint32_t h)
{
    k *= 0xcc9e2d51;
    k = (k >> 17) | (k << 15);
    k *= 0x1b873593;
    h ^= k;
    h = (h >> 19) | (h << 13);
    return ((h * 5) + 0xe6546b64);
}

/**
 * @brief Encodes a letter into a 5 bit value for packing.
 *
 * A to Z and a to z map to 1 to 26, any character below A maps to 27.
 * The result only fits in 5 bit for the ASCII range, as documented for encode_refalt.
 *
 * @param c  Character to encode.
 *
 * @return The encoded value.
 */
static inline uint32_t encode_packchar(int c)
{
    // 256 byte lookup table indexed by (uint8_t)c, so the result does not depend
    // on the signedness of plain char.
    // *INDENT-OFF*
    static const uint8_t map[256] =
    {
        27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
        27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
        27,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
         0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
        27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
        27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
        27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
        27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
    };
    // *INDENT-ON*
    return (uint32_t)map[(uint8_t)c];
}

/**
 * @brief Packs up to 5 characters into a 32 bit unsigned integer.
 *
 * The characters occupy the same bit positions they would have in pack_chars:
 * (6 x 5 bit + 2 spare bit) [ 01111122 22233333 44444555 55666660 ].
 *
 * @param str  String to pack.
 * @param size Length of the string, from 0 to 5, excluding the terminating null byte.
 *
 * @return The packed characters.
 */
static inline uint32_t pack_chars_tail(const char *str, size_t size)
{
    uint32_t h = 0;
    const char *pos = (str + size);
    switch (size)
    {
    case 5:
        h ^= encode_packchar(*(--pos)) << (1 + (5 * 1));
    // fall through
    case 4:
        h ^= encode_packchar(*(--pos)) << (1 + (5 * 2));
    // fall through
    case 3:
        h ^= encode_packchar(*(--pos)) << (1 + (5 * 3));
    // fall through
    case 2:
        h ^= encode_packchar(*(--pos)) << (1 + (5 * 4));
    // fall through
    case 1:
        h ^= encode_packchar(*(--pos)) << (1 + (5 * 5));
    default:
        break;
    }
    return h;
}

/**
 * @brief Packs 6 characters into a 32 bit unsigned integer.
 *
 * @param str  String to pack (it must contain 6 characters at least).
 *
 * @return The packed characters.
 */
static inline uint32_t pack_chars(const char *str)
{
    const char *pos = (str + 5);
    return ((encode_packchar(*pos) << 1)
            ^ (encode_packchar(*(pos-1)) << (1 + (5 * 1)))
            ^ (encode_packchar(*(pos-2)) << (1 + (5 * 2)))
            ^ (encode_packchar(*(pos-3)) << (1 + (5 * 3)))
            ^ (encode_packchar(*(pos-4)) << (1 + (5 * 4)))
            ^ (encode_packchar(*(pos-5)) << (1 + (5 * 5))));
}

/**
 * @brief Hashes a string into a 32 bit unsigned integer.
 *
 * The string is consumed in blocks of 6 packed characters.
 *
 * @param str  String to hash.
 * @param size Length of the string, excluding the terminating null byte.
 *
 * @return The 32 bit hash.
 */
static inline uint32_t hash32(const char *str, size_t size)
{
    uint32_t h = 0;
    size_t len = 6;
    while (size >= len)
    {
        h = muxhash(pack_chars(str), h);
        str += len;
        size -= len;
    }
    if (size > 0)
    {
        h = muxhash(pack_chars_tail(str, size), h);
    }
    return h;
}

/**
 * @brief Encodes a REF+ALT pair as a non-reversible 31 bit hash.
 *
 * The least significant bit of the result is always set to mark the hash mode.
 *
 * @param ref      Reference allele string.
 * @param sizeref  Length of the ref string, excluding the terminating null byte.
 * @param alt      Alternate allele string.
 * @param sizealt  Length of the alt string, excluding the terminating null byte.
 *
 * @return The 32 bit REF+ALT code.
 */
static inline uint32_t encode_refalt_hash(const char *ref, size_t sizeref, const char *alt, size_t sizealt)
{
    // 0x3 is the separator character between REF and ALT [00000000 00000000 00000000 00000011]
    uint32_t h = muxhash(hash32(alt, sizealt), muxhash(0x3, hash32(ref, sizeref)));
    // MurmurHash3 finalization mix - force all bits of a hash block to avalanche
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return ((h >> 1) | 0x1); // 0x1 is the set bit to indicate HASH mode [00000000 00000000 00000000 00000001]
}

/**
 * @brief Encodes a REF+ALT pair into a 31 bit code.
 *
 * The reversible encoding is used when each allele is VKMAX_ALLELE_LEN bases or
 * less, the two alleles together are VKMAX_REFALT_LEN bases or less and they
 * contain only A, C, G and T, otherwise the pair is hashed.
 *
 * @param ref      Reference allele. Characters must be A-Z, a-z or *.
 * @param sizeref  Length of the ref string, excluding the terminating null byte.
 * @param alt      Alternate non-reference allele. Characters must be A-Z, a-z or *.
 * @param sizealt  Length of the alt string, excluding the terminating null byte.
 *
 * @return The 32 bit REF+ALT code.
 */
static inline uint32_t encode_refalt(const char *ref, size_t sizeref, const char *alt, size_t sizealt)
{
    // The length of a single allele is checked as well: the length fields hold
    // up to VKMAX_REFALT_LEN, but decode_refalt only reverses the codes whose
    // alleles are within VKMAX_ALLELE_LEN.
    if (((sizeref + sizealt) <= VKMAX_REFALT_LEN) && (sizeref <= VKMAX_ALLELE_LEN) && (sizealt <= VKMAX_ALLELE_LEN))
    {
        uint32_t h = encode_refalt_rev(ref, sizeref, alt, sizealt);
        if (h != MAXUINT32)
        {
            return h;
        }
    }
    return encode_refalt_hash(ref, sizeref, alt, sizealt);
}

/**
 * @brief Decodes the base stored at the given bit position of a REF+ALT code.
 *
 * @param code     REF+ALT code.
 * @param bitpos   Bit position of the base.
 *
 * @return The base letter: A, C, G or T.
 */
static inline char decode_base(uint32_t code, uint8_t bitpos)
{
    static const char base[4] = {'A', 'C', 'G', 'T'};
    return base[((code >> (bitpos & 0x1F)) & 0x3)]; // 0x3 is the 2 bit mask [00000011]
}

/**
 * @brief Decodes a REF+ALT code produced by the reversible encoding.
 *
 * The caller must check that the code carries lengths within the reversible
 * range, as decode_refalt does.
 *
 * @param code     REF+ALT code.
 * @param ref      REF string buffer to be returned (it must be sized VKMAX_ALLELE_LEN + 1 bytes at least).
 * @param sizeref  Pointer to the returned length of the ref string, excluding the terminating null byte.
 * @param alt      ALT string buffer to be returned (it must be sized VKMAX_ALLELE_LEN + 1 bytes at least).
 * @param sizealt  Pointer to the returned length of the alt string, excluding the terminating null byte.
 *
 * @return The total number of REF+ALT characters.
 */
static inline size_t decode_refalt_rev(uint32_t code, char *ref, size_t *sizeref, char *alt, size_t *sizealt)
{
    *sizeref = (size_t)((code & 0x78000000) >> 27); // [01111000 00000000 00000000 00000000]
    *sizealt = (size_t)((code & 0x07800000) >> 23); // [00000111 10000000 00000000 00000000]
    switch (*sizeref)
    {
    case 10:
        ref[9] = decode_base(code, (3 + (2 * 0)));
    // fall through
    case 9:
        ref[8] = decode_base(code, (3 + (2 * 1)));
    // fall through
    case 8:
        ref[7] = decode_base(code, (3 + (2 * 2)));
    // fall through
    case 7:
        ref[6] = decode_base(code, (3 + (2 * 3)));
    // fall through
    case 6:
        ref[5] = decode_base(code, (3 + (2 * 4)));
    // fall through
    case 5:
        ref[4] = decode_base(code, (3 + (2 * 5)));
    // fall through
    case 4:
        ref[3] = decode_base(code, (3 + (2 * 6)));
    // fall through
    case 3:
        ref[2] = decode_base(code, (3 + (2 * 7)));
    // fall through
    case 2:
        ref[1] = decode_base(code, (3 + (2 * 8)));
    // fall through
    case 1:
        ref[0] = decode_base(code, (3 + (2 * 9)));
        break;
    default:
        break;
    }
    ref[*sizeref] = 0;
    uint8_t bitpos = (23 - ((*sizeref) << 1));
    switch (*sizealt)
    {
    case 10:
        alt[9] = decode_base(code, bitpos - (2 * 10));
    // fall through
    case 9:
        alt[8] = decode_base(code, bitpos - (2 * 9));
    // fall through
    case 8:
        alt[7] = decode_base(code, bitpos - (2 * 8));
    // fall through
    case 7:
        alt[6] = decode_base(code, bitpos - (2 * 7));
    // fall through
    case 6:
        alt[5] = decode_base(code, bitpos - (2 * 6));
    // fall through
    case 5:
        alt[4] = decode_base(code, bitpos - (2 * 5));
    // fall through
    case 4:
        alt[3] = decode_base(code, bitpos - (2 * 4));
    // fall through
    case 3:
        alt[2] = decode_base(code, bitpos - (2 * 3));
    // fall through
    case 2:
        alt[1] = decode_base(code, bitpos - (2 * 2));
    // fall through
    case 1:
        alt[0] = decode_base(code, bitpos - (2 * 1));
        break;
    default:
        break;
    }
    alt[*sizealt] = 0;
    return (*sizeref + *sizealt);
}

/**
 * @brief Decodes a 32 bit REF+ALT code if it was produced by the reversible encoding.
 *
 * The code is reversible when its least significant bit is clear and the two
 * length fields are within VKMAX_ALLELE_LEN and VKMAX_REFALT_LEN.
 *
 * @param code     REF+ALT code.
 * @param ref      REF string buffer to be returned (it must be sized VKMAX_ALLELE_LEN + 1 bytes at least).
 * @param sizeref  Pointer to the returned length of the ref string, excluding the terminating null byte.
 * @param alt      ALT string buffer to be returned (it must be sized VKMAX_ALLELE_LEN + 1 bytes at least).
 * @param sizealt  Pointer to the returned length of the alt string, excluding the terminating null byte.
 *
 * @return The total number of REF+ALT characters, or 0 if the code is not reversible.
 */
static inline size_t decode_refalt(uint32_t code, char *ref, size_t *sizeref, char *alt, size_t *sizealt)
{
    if (code & 0x1) // check last bit
    {
        return 0; // non-reversible encoding
    }
    size_t lenref = (size_t)((code & 0x78000000) >> 27); // [01111000 00000000 00000000 00000000]
    size_t lenalt = (size_t)((code & 0x07800000) >> 23); // [00000111 10000000 00000000 00000000]
    if ((lenref > VKMAX_ALLELE_LEN) || (lenalt > VKMAX_ALLELE_LEN) || ((lenref + lenalt) > VKMAX_REFALT_LEN))
    {
        return 0; // the length fields are out of the range of a reversible encoding
    }
    return decode_refalt_rev(code, ref, sizeref, alt, sizealt);
}

/**
 * @brief Assembles a VariantKey from the pre-encoded CHROM, POS and REF+ALT.
 *
 * @param chrom      Encoded chromosome (see encode_chrom).
 * @param pos        Reference position, with the first base having position 0.
 * @param refalt     Encoded REF+ALT (see encode_refalt).
 *
 * @return VariantKey 64 bit code.
 */
static inline uint64_t encode_variantkey(uint8_t chrom, uint32_t pos, uint32_t refalt)
{
    return (((uint64_t)chrom << VKSHIFT_CHROM) | ((uint64_t)pos << VKSHIFT_POS) | (uint64_t)refalt);
}

/**
 * @brief Extracts the CHROM code from a VariantKey.
 *
 * @param vk VariantKey code.
 *
 * @return CHROM code.
 */
static inline uint8_t extract_variantkey_chrom(uint64_t vk)
{
    return (uint8_t)((vk & VKMASK_CHROM) >> VKSHIFT_CHROM);
}

/**
 * @brief Extracts the POS value from a VariantKey.
 *
 * @param vk VariantKey code.
 *
 * @return POS.
 */
static inline uint32_t extract_variantkey_pos(uint64_t vk)
{
    return (uint32_t)((vk & VKMASK_POS) >> VKSHIFT_POS);
}

/**
 * @brief Extracts the REF+ALT code from a VariantKey.
 *
 * @param vk VariantKey code.
 *
 * @return REF+ALT code.
 */
static inline uint32_t extract_variantkey_refalt(uint64_t vk)
{
    return (uint32_t)(vk & VKMASK_REFALT);
}

/**
 * @brief Splits a VariantKey into its CHROM, POS and REF+ALT components.
 *
 * @param code VariantKey code.
 * @param vk   Structure containing the return values.
 */
static inline void decode_variantkey(uint64_t code, variantkey_t *vk)
{
    vk->chrom = extract_variantkey_chrom(code);
    vk->pos = extract_variantkey_pos(code);
    vk->refalt = extract_variantkey_refalt(code);
}

/**
 * @brief Returns a VariantKey for the given CHROM, POS (0-based), REF and ALT.
 *
 * The variant should be already normalized (see normalize_variant or use normalized_variantkey).
 *
 * @param chrom      Chromosome identifier, no white-space or leading zeros permitted.
 * @param sizechrom  Length of the chrom string, excluding the terminating null byte.
 * @param pos        Reference position, with the first base having position 0.
 * @param ref        Reference allele. Characters must be A-Z, a-z or *.
 * @param sizeref    Length of the ref string, excluding the terminating null byte.
 * @param alt        Alternate non-reference allele. Characters must be A-Z, a-z or *.
 * @param sizealt    Length of the alt string, excluding the terminating null byte.
 *
 * @return VariantKey 64 bit code.
 */
static inline uint64_t variantkey(const char *chrom, size_t sizechrom, uint32_t pos, const char *ref, size_t sizeref, const char *alt, size_t sizealt)
{
    return encode_variantkey(encode_chrom(chrom, sizechrom), pos, encode_refalt(ref, sizeref, alt, sizealt));
}

/**
 * @brief Returns the minimum and maximum VariantKey of a CHROM and POS range.
 *
 * @param chrom     Encoded chromosome number.
 * @param pos_min   Start reference position, with the first base having position 0.
 * @param pos_max   End reference position, with the first base having position 0.
 * @param range     Structure containing the return values.
 */
static inline void variantkey_range(uint8_t chrom, uint32_t pos_min, uint32_t pos_max, vkrange_t *range)
{
    uint64_t c = ((uint64_t)chrom << VKSHIFT_CHROM);
    range->min = (c | ((uint64_t)pos_min << VKSHIFT_POS));
    range->max = (c | ((uint64_t)pos_max << VKSHIFT_POS) | VKMASK_REFALT);
}

/**
 * @brief Compares two unsigned 64 bit integers.
 *
 * @param a  First integer.
 * @param b  Second integer.
 *
 * @return -1 if a < b, 0 if a == b, 1 if a > b.
 */
static inline int8_t compare_uint64_t(uint64_t a, uint64_t b)
{
    return (int8_t)((a > b) - (a < b));
}

/**
 * @brief Compares two VariantKeys by CHROM only.
 *
 * @param vka    First VariantKey.
 * @param vkb    Second VariantKey.
 *
 * @return -1 if vka is smaller, 0 if they are equal, 1 if vka is greater.
 */
static inline int8_t compare_variantkey_chrom(uint64_t vka, uint64_t vkb)
{
    return compare_uint64_t((vka >> VKSHIFT_CHROM), (vkb >> VKSHIFT_CHROM));
}

/**
 * @brief Compares two VariantKeys by CHROM and POS.
 *
 * @param vka    First VariantKey.
 * @param vkb    Second VariantKey.
 *
 * @return -1 if vka is smaller, 0 if they are equal, 1 if vka is greater.
 */
static inline int8_t compare_variantkey_chrom_pos(uint64_t vka, uint64_t vkb)
{
    return compare_uint64_t((vka >> VKSHIFT_POS), (vkb >> VKSHIFT_POS));
}

/**
 * @brief Writes a VariantKey as a 16 character hexadecimal string.
 *
 * @param vk    VariantKey code.
 * @param str   String buffer to be returned (it must be sized 17 bytes at least).
 *
 * @return The number of characters written, excluding the terminating null byte.
 */
static inline size_t variantkey_hex(uint64_t vk, char *str)
{
    return hex_uint64_t(vk, str);
}

/**
 * @brief Parses a 16 character hexadecimal string into a VariantKey.
 *
 * @param vs    VariantKey hexadecimal string (it must contain 16 hexadecimal characters).
 *
 * @return VariantKey 64 bit code.
 */
static inline uint64_t parse_variantkey_hex(const char *vs)
{
    return parse_hex_uint64_t(vs);
}

#endif  // VARIANTKEY_VARIANTKEY_H
