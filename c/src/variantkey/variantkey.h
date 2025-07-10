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
 * @brief VariantKey main functions.
 *
 * The functions provided here allows to generate and process a 64 bit Unsigned Integer Keys for Human Genetic Variants.
 * The VariantKey is sortable for chromosome and position,
 * and it is also fully reversible for variants with up to 11 bases between Reference and Alternate alleles.
 * It can be used to sort, search and match variant-based data easily and very quickly.
 */

#ifndef VARIANTKEY_VARIANTKEY_H
#define VARIANTKEY_VARIANTKEY_H

#include <inttypes.h>
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

/**
 * VariantKey struct.
 * Contains the numerically encoded VariantKey components (CHROM, POS, REF+ALT).
 */
typedef struct variantkey_t
{
    uint8_t chrom;   //!< Chromosome encoded number (only the LSB 5 bit are used)
    uint32_t pos;    //!< Reference position, with the first base having position 0 (only the LSB 28 bit are used)
    uint32_t refalt; //!< Code for Reference and Alternate allele (only the LSB 31 bits are used)
} variantkey_t;

/**
 * Struct containing the minimum and maximum VariantKey values for range searches.
 */
typedef struct vkrange_t
{
    uint64_t min; //!< Minimum VariantKey value for any given REF+ALT encoding
    uint64_t max; //!< Maximum VariantKey value for any given REF+ALT encoding
} vkrange_t;

/** @brief Returns the encoding for a numerical chromosome input.
 *
 * This function encodes a chromosome string that contains only digits (0-9).
 * It processes each character of the chromosome string,
 * converting it into a numerical value.
 *
 * @param chrom  Chromosome. An identifier from the reference genome, no white-space permitted.
 * @param size   Length of the chrom string, excluding the terminating null byte.
 *
 * @return CHROM code
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


/** @brief Returns a true value (1) if the input chrom has 'chr' or 'CHR' prefix.
 *
 * This function checks if the chromosome string starts with the common prefix "chr" or "CHR".
 *
 * @param chrom  Chromosome. An identifier from the reference genome, no white-space permitted.
 * @param size   Length of the chrom string, excluding the terminating null byte.
 *
 * @return True (1) if the 'chr' prefix is present.
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

/** @brief Returns chromosome numerical encoding.
 *
 * This function encodes a chromosome string into a numerical code.
 * It processes the chromosome string to handle both numeric chromosomes (0-22) and special cases for X, Y, and M.
 * It also checks for the common prefix "chr" or "CHR" and removes it if present.
 * If the chromosome string is empty or contains invalid characters, it returns 0 (NA).
 *
 * @param chrom  Chromosome. An identifier from the reference genome, no white-space permitted.
 * @param size   Length of the chrom string, excluding the terminating null byte.
 *
 * @return CHROM code or 0 in case of invalid input.
 */
static inline uint8_t encode_chrom(const char *chrom, size_t size)
{
    // X = 23; Y = 24; M = 25; any other letter is mapped to 0:
    static const uint8_t onecharmap[] =
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        /*                                    M                                X  Y                  */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,25, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,23,24, 0, 0, 0, 0, 0, 0,
        /*                                    m                                x  y                  */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,25, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,23,24, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };
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

/** @brief Decode the chromosome numerical code.
 *
 * This function decodes a numerical chromosome code into a string representation.
 * It handles numeric chromosomes (1-22) and special cases for X, Y, and M.
 * It also returns 'NA' for invalid codes.
 *
 * @param code   CHROM code.
 * @param chrom  CHROM string buffer to be returned. Its size should be enough to contain the results (max 4 bytes).
 *
 * @return If successful, the total number of characters written is returned,
 *         excluding the null-character appended at the end of the string,
 *         otherwise a negative number is returned in case of failure.
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

/** @brief Encodes a nucleotide base into a numerical code.
 *
 * This function encodes a nucleotide character into a 2-bit numerical code.
 * The mapping is as follows:
 * - A = 0
 * - C = 1
 * - G = 2
 * - T = 3
 * Invalid characters are mapped to 4.
 *
 * @param c  Nucleotide character to encode.
 *
 * @return Numerical code for the nucleotide base (A=0, C=1, G=2, T=3, invalid=4).
 */
static inline uint32_t encode_base(const uint8_t c)
{
    static const uint32_t map[] =
    {
        4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
        4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
        /*A   C       G                         T*/
        4,0,4,1,4,4,4,2,4,4,4,4,4,4,4,4,4,4,4,4,3,4,4,4,4,4,4,4,4,4,4,4,
        /*a   c       g                         t*/
        4,0,4,1,4,4,4,2,4,4,4,4,4,4,4,4,4,4,4,4,3,4,4,4,4,4,4,4,4,4,4,4,
        4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
        4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
        4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
        4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
    };
    return map[c];
}

/** @brief Encodes a nucleotide allele into an integer.
 *
 * This function encodes a nucleotide allele into an integer.
 * It processes each character of the allele string,
 * encoding it into a 2 bit value (A=0, C=1, G=2, T=3).
 * If an invalid base is encountered (i.e., a base that is not A, C, G, or T),
 * it returns -1 to indicate an error.
 *
 * @param h        Pointer to the 32 bit hash where the encoded allele will be stored.
 * @param bitpos   Pointer to the current bit position in the hash.
 * @param str      Pointer to the allele string to be encoded.
 * @param size     Size of the allele string, excluding the terminating null byte.
 *
 * @return         0 on success, or -1 if an invalid base is encountered (i.e., a base that is not A, C, G, or T).
 */
static inline int encode_allele(uint32_t *h, uint8_t *bitpos, const char *str, size_t size)
{
    uint32_t v = 0;
    while (size--)
    {
        v = encode_base(*str++);
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
 * @brief Encodes a REF+ALT pair.
 *
 * This function encodes a REF+ALT pair into a 32 bit unsigned integer.
 * It uses a reversible encoding scheme for REF and ALT alleles,
 * where each allele is represented by its length and the nucleotide bases.
 * The encoding is reversible for alleles with a total length of 11 or less bases.
 *
 * @param ref      Reference allele string.
 * @param sizeref  Length of the reference allele string, excluding the terminating null byte.
 * @param alt      Alternate allele string.
 * @param sizealt  Length of the alternate allele string, excluding the terminating null byte.
 *
 * @return         A 32 bit unisgned integer representing the encoded REF+ALT pair.
 *                 If an error occurs during encoding (e.g., invalid base), it returns MAXUINT32.
 */
static inline uint32_t encode_refalt_rev(const char *ref, size_t sizeref, const char *alt, size_t sizealt)
{
    // [******** ******** ******** ******** *RRRRAAA A1122334 45566778 8990011*]
    // RRRR: length of (REF - 1) ; AAAA: length of (ALT - 1)
    uint32_t h = ((uint32_t)(sizeref) << 27) |((uint32_t)(sizealt) << 23);
    uint8_t bitpos = 23;
    if ((encode_allele(&h, &bitpos, ref, sizeref) < 0) || (encode_allele(&h, &bitpos, alt, sizealt) < 0))
    {
        return MAXUINT32; // error code
    }
    return h;
}

/** @brief Mix two 32 bit hash numbers using a MurmurHash3-like algorithm.
 *
 * This function mixes two 32 bit hash numbers using a MurmurHash3-like algorithm.
 * It takes a key (k) and a hash (h) as input,
 * and applies a series of bitwise operations and multiplications to mix the two values.
 * The mixing process ensures that the resulting hash is well-distributed and has good avalanche properties.
 *
 * @param k  The key to be mixed.
 * @param h  The hash to be mixed with the key.
 *
 * @return   The mixed hash value.
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
 * @brief Encodes a character for packing.
 *
 * This function encodes a character into a 5 bit value.
 * It maps characters 'A' to 'Z' and 'a' to 'z' to values 1 to 26.
 * Invalid characters (i.e., characters not in the range 'A'-'Z' or 'a'-'z') are mapped to 27.
 *
 * @param c  The character to be encoded.
 *
 * @return   A 5 bit encoded value for the character.
 */
static inline uint32_t encode_packchar(int c)
{
    if (c < 'A')
    {
        return 27;
    }
    return (uint32_t)((c | 0x20) - 'a' + 1);
}

/** @brief Packs a string of characters 1 to 5 characters.
 *
 * This function packs a string of max 5 characters in 32 bit unsigned integer:
 * (6 x 5 bit + 2 spare bit) [ 01111122 22233333 44444555 55666660 ].
 *
 * @param str  Pointer to the string of characters to be packed.
 * @param size Length of the string, excluding the terminating null byte.
 *
 * @return     A 32 bit unsigned integer containing the packed characters.
 */
static inline uint32_t pack_chars_tail(const char *str, size_t size)
{
    uint32_t h = 0;
    const char *pos = (str + size - 1);
    switch (size)
    {
    case 5:
        h ^= encode_packchar(*pos--) << (1 + (5 * 1));
    // fall through
    case 4:
        h ^= encode_packchar(*pos--) << (1 + (5 * 2));
    // fall through
    case 3:
        h ^= encode_packchar(*pos--) << (1 + (5 * 3));
    // fall through
    case 2:
        h ^= encode_packchar(*pos--) << (1 + (5 * 4));
    // fall through
    case 1:
        h ^= encode_packchar(*pos) << (1 + (5 * 5));
    default:
        break;
    }
    return h;
}

/**
 * @brief Packs a string of 6 characters into a 32 bit unsigned integer.
 *
 * This function packs a string of 6 characters into a 32 bit unsigned integer.
 * It encodes each character into a 5 bit value and combines them into a single integer.
 * The characters are packed in reverse order, starting from the 6th character.
 *
 * @param str  Pointer to the string of characters to be packed.
 *
 * @return     A 32 bit unsigned integer containing the packed characters.
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

/** @brief Packs a nucleotide string into a 32 bit hash.
 *
 * Return a 32 bit hash of a nucleotide string.
 *
 * @param str  Pointer to the nucleotide string to be packed.
 * @param size Length of the nucleotide string, excluding the terminating null byte.
 *
 * @return     A 32 bit unsigned integer containing the hashed characters.
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
 * @brief Encodes a REF+ALT pair into a 32 bit hash.
 *
 * This function encodes a REF+ALT pair into a 32 bit hash.
 * It uses a MurmurHash3-like algorithm to mix the hashes of the REF and ALT alleles.
 *
 * @param ref      Reference allele string.
 * @param sizeref  Length of the reference allele string, excluding the terminating null byte.
 * @param alt      Alternate allele string.
 * @param sizealt  Length of the alternate allele string, excluding the terminating null byte.
 *
 * @return         A 32 bit unsigned integer representing the encoded REF+ALT pair.
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

/** @brief Returns reference+alternate numerical encoding.
 *
 * This function encodes a REF+ALT pair into a 32 bit unsigned integer.
 * It uses a reversible encoding scheme for REF and ALT alleles,
 * when the total length of REF and ALT is 11 or less bases.
 * If the total length of REF and ALT is greater than 11, it uses a hashing scheme.
 *
 * @param ref      Reference allele. String containing a sequence of nucleotide letters.
 *                 The value in the pos field refers to the position of the first nucleotide in the String.
 *                 Characters must be A-Z, a-z or *
 * @param sizeref  Length of the ref string, excluding the terminating null byte.
 * @param alt      Alternate non-reference allele string.
 *                 Characters must be A-Z, a-z or *
 * @param sizealt  Length of the alt string, excluding the terminating null byte.
 *
 * @return REF+ALT code
 */
static inline uint32_t encode_refalt(const char *ref, size_t sizeref, const char *alt, size_t sizealt)
{
    if ((sizeref + sizealt) <= 11)
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
 * @brief Decodes a base from a 32 bit code.
 *
 * This function decodes a base from a 32 bit code.
 * It extracts the base value from the specified bit position in the code.
 * The base is represented as a character (A, C, G, T).
 *
 * @param code     The 32 bit code containing the base information.
 * @param bitpos   The bit position from which to extract the base value.
 *
 * @return         The decoded base character (A, C, G, T).
 */
static inline char decode_base(uint32_t code, int bitpos)
{
    static const char base[4] = {'A', 'C', 'G', 'T'};
    return base[((code >> bitpos) & 0x3)]; // 0x3 is the 2 bit mask [00000011]
}

/**
 *
 * @brief Decodes a REF+ALT code if reversible (if it has 11 or less bases in total and only contains ACGT letters).
 *
 * This function decodes a REF+ALT code into its constituent reference and alternate alleles.
 * It extracts the sizes of the reference and alternate alleles,
 * and then decodes the bases from the code.
 * The function assumes that the code is reversible, meaning it has 11 or fewer bases in total
 * and only contains valid nucleotide letters (A, C, G, T).
 *
 * @param code     REF+ALT code.
 * @param ref      Reference allele string buffer to be returned.
 * @param sizeref  Pointer to the size of the ref buffer, excluding the terminating null byte.
 *                 This will contain the final ref size.
 * @param alt      Alternate allele string buffer to be returned.
 * @param sizealt  Pointer to the size of the alt buffer, excluding the terminating null byte.
 *                 This will contain the final alt size.
 * @return         If the code is reversible, then the total number of characters of REF+ALT is returned.
 *                 Otherwise 0 is returned.
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
    }
    alt[*sizealt] = 0;
    return (*sizeref + *sizealt);
}

/** @brief Decode the 32 bit REF+ALT code if reversible (if it has 11 or less bases in total and only contains ACGT letters).
 *
 * This function decodes a REF+ALT code into its constituent reference and alternate alleles.
 * It checks if the code is reversible by examining the last bit.
 * If the last bit is set, it indicates a non-reversible encoding, and the function returns 0.
 * If the last bit is not set, it calls decode_refalt_rev to decode the REF+ALT pair.
 *
 * @param code     REF+ALT code
 * @param ref      REF string buffer to be returned.
 * @param sizeref  Pointer to the size of the ref buffer, excluding the terminating null byte.
 *                 This will contain the final ref size.
 * @param alt      ALT string buffer to be returned.
 * @param sizealt  Pointer to the size of the alt buffer, excluding the terminating null byte.
 *                 This will contain the final alt size.
 *
 * @return      If the code is reversible, then the total number of characters of REF+ALT is returned.
 *              Otherwise 0 is returned.
 */
static inline size_t decode_refalt(uint32_t code, char *ref, size_t *sizeref, char *alt, size_t *sizealt)
{
    if (code & 0x1) // check last bit
    {
        return 0; // non-reversible encoding
    }
    return decode_refalt_rev(code, ref, sizeref, alt, sizealt);
}

/** @brief Returns a 64 bit variant key based on the pre-encoded CHROM, POS (0-based) and REF+ALT.
 *
 * This function encodes a variant key using the provided pre-encoded chromosome, position, and reference+alternate values.
 *
 * @param chrom      Encoded Chromosome (see encode_chrom).
 * @param pos        Position. The reference position, with the first base having position 0.
 * @param refalt     Encoded Reference + Alternate (see encode_refalt).
 *
 * @return      VariantKey 64 bit code.
 */
static inline uint64_t encode_variantkey(uint8_t chrom, uint32_t pos, uint32_t refalt)
{
    return (((uint64_t)chrom << VKSHIFT_CHROM) | ((uint64_t)pos << VKSHIFT_POS) | (uint64_t)refalt);
}

/** @brief Extract the CHROM code from VariantKey.
 *
 * This function extracts the chromosome code from a VariantKey.
 *
 * @param vk VariantKey code.
 *
 * @return CHROM code.
 */
static inline uint8_t extract_variantkey_chrom(uint64_t vk)
{
    return (uint8_t)((vk & VKMASK_CHROM) >> VKSHIFT_CHROM);
}

/** @brief Extract the POS code from VariantKey.
 *
 * This function extracts the position code from a VariantKey.
 *
 * @param vk VariantKey code.
 *
 * @return POS.
 */
static inline uint32_t extract_variantkey_pos(uint64_t vk)
{
    return (uint32_t)((vk & VKMASK_POS) >> VKSHIFT_POS);
}

/** @brief Extract the REF+ALT code from VariantKey.
 *
 * This function extracts the reference and alternate alleles from a VariantKey.
 *
 * @param vk VariantKey code.
 *
 * @return REF+ALT code.
 */
static inline uint32_t extract_variantkey_refalt(uint64_t vk)
{
    return (uint32_t)(vk & VKMASK_REFALT);
}

/** @brief Decode a VariantKey code and returns the components as variantkey_t structure.
 *
 * This function decodes a VariantKey code into its constituent components:
 * chromosome, position, and reference+alternate alleles.
 *
 * @param code VariantKey code.
 * @param vk   Decoded variantkey structure.
 */
static inline void decode_variantkey(uint64_t code, variantkey_t *vk)
{
    vk->chrom = extract_variantkey_chrom(code);
    vk->pos = extract_variantkey_pos(code);
    vk->refalt = extract_variantkey_refalt(code);
}

/** @brief Returns a 64 bit variantkey based on CHROM, POS (0-based), REF, ALT.
 *
 * Returns a 64 bit variantkey based on CHROM, POS (0-based), REF, ALT.
 * The variant should be already normalized (see normalize_variant or use normalized_variantkey).
 *
 * @param chrom      Chromosome. An identifier from the reference genome, no white-space or leading zeros permitted.
 * @param sizechrom  Length of the chrom string, excluding the terminating null byte.
 * @param pos        Position. The reference position, with the first base having position 0.
 * @param ref        Reference allele. String containing a sequence of nucleotide letters.
 *                   The value in the pos field refers to the position of the first nucleotide in the String.
 *                   Characters must be A-Z, a-z or *
 * @param sizeref    Length of the ref string, excluding the terminating null byte.
 * @param alt        Alternate non-reference allele string.
 *                   Characters must be A-Z, a-z or *
 * @param sizealt    Length of the alt string, excluding the terminating null byte.
 *
 * @return      VariantKey 64 bit code.
 */
static inline uint64_t variantkey(const char *chrom, size_t sizechrom, uint32_t pos, const char *ref, size_t sizeref, const char *alt, size_t sizealt)
{
    return encode_variantkey(encode_chrom(chrom, sizechrom), pos, encode_refalt(ref, sizeref, alt, sizealt));
}

/** @brief Returns minimum and maximum VariantKeys for range searches.
 *
 * This function computes the minimum and maximum VariantKeys for a given range.
 * It takes the chromosome, start position, end position, and a pointer to a vkrange_t structure.
 * The function sets the min and max fields of the vkrange_t structure
 * to the computed minimum and maximum VariantKeys.
 *
 * @param chrom     Chromosome encoded number.
 * @param pos_min   Start reference position, with the first base having position 0.
 * @param pos_max   End reference position, with the first base having position 0.
 * @param range     VariantKey range values.
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
 * This function compares two unsigned 64 bit integers and returns:
 * - -1 if the first integer is less than the second,
 * - 0 if they are equal,
 * - 1 if the first integer is greater than the second.
 *
 * @param a  The first unsigned 64 bit integer to be compared.
 * @param b  The second unsigned 64 bit integer to be compared.
 *
 * @return -1 if a < b, 0 if a == b, 1 if a > b.
 */
static inline int8_t compare_uint64_t(uint64_t a, uint64_t b)
{
    return (a < b) ? -1 : (a > b); // NOLINT(bugprone-narrowing-conversions,cppcoreguidelines-narrowing-conversions)
}

/** @brief Compares two VariantKeys by chromosome only.
 *
 * This function compares two VariantKeys by their chromosome values.
 * The function returns:
 * - -1 if the first chromosome is smaller than the second,
 * - 0 if they are equal,
 * - 1 if the first is greater than the second.
 *
 * @param vka    The first VariantKey to be compared.
 * @param vkb    The second VariantKey to be compared.
 *
 * @return -1 if the first chromosome is smaller than the second, 0 if they are equal and 1 if the first is greater than the second.
 */
static inline int8_t compare_variantkey_chrom(uint64_t vka, uint64_t vkb)
{
    return compare_uint64_t((vka >> VKSHIFT_CHROM), (vkb >> VKSHIFT_CHROM));
}

/** @brief Compares two VariantKeys by chromosome and position.
 *
 * This function compares two VariantKeys by their chromosome and position values.
 * The function returns:
 * - -1 if the first CHROM+POS is smaller than the second,
 * - 0 if they are equal,
 * - 1 if the first is greater than the second.
 *
 * @param vka    The first VariantKey to be compared.
 * @param vkb    The second VariantKey to be compared.
 *
 * @return -1 if the first CHROM+POS is smaller than the second, 0 if they are equal and 1 if the first is greater than the second.
 */
static inline int8_t compare_variantkey_chrom_pos(uint64_t vka, uint64_t vkb)
{
    return compare_uint64_t((vka >> VKSHIFT_POS), (vkb >> VKSHIFT_POS));
}

/** @brief Returns VariantKey hexadecimal string (16 characters).
 *
 * This function converts a VariantKey code into a hexadecimal string representation.
 * The string represent a 64 bit number or:
 *   -  5 bit for CHROM
 *   - 28 bit for POS
 *   - 31 bit for REF+ALT
 *
 * @param vk    VariantKey code.
 * @param str   String buffer to be returned (it must be sized 17 bytes at least).
 *
 * @return      Upon successful return, these function returns the number of characters processed
 *              (excluding the null byte used to end output to strings).
 *              If the buffer size is not sufficient, then the return value is the number of characters required for
 *              buffer string, including the terminating null byte.
 */
static inline size_t variantkey_hex(uint64_t vk, char *str)
{
    return hex_uint64_t(vk, str);
}

/** @brief Parses a VariantKey hexadecimal string and returns the code.
 *
 * This function parses a hexadecimal string representation of a VariantKey
 * and converts it into a 64 bit unsigned integer.
 *
 * @param vs    VariantKey hexadecimal string (it must contain 16 hexadecimal characters).
 *
 * @return A VariantKey code.
 */
static inline uint64_t parse_variantkey_hex(const char *vs)
{
    return parse_hex_uint64_t(vs);
}

#endif  // VARIANTKEY_VARIANTKEY_H
