// RegionKey
//
// regionkey.h
//
// @category   Libraries
// @author     Nicola Asuni <info@tecnick.com>
// @link       https://github.com/tecnickcom/variantkey
// @license    MIT [LICENSE](https://raw.githubusercontent.com/tecnickcom/variantkey/main/LICENSE)

/**
 * @file regionkey.h
 * @brief Encoding and decoding of the 64 bit RegionKey.
 *
 * A RegionKey packs a chromosome in 5 bit, a start position in 28 bit, an end
 * position in 28 bit and a strand in 2 bit. Keys sort by chromosome and start
 * position, and are fully reversible.
 */

#ifndef VARIANTKEY_REGIONKEY_H
#define VARIANTKEY_REGIONKEY_H

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include "variantkey.h"
#include "nrvk.h"
#include "hex.h"

#define RK_MAX_POS       0x000000000FFFFFFF  //!< Maximum position value (2^28 - 1)
#define RKMASK_CHROM     0xF800000000000000  //!< RegionKey binary mask for CHROM     [ 11111000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 ]
#define RKMASK_STARTPOS  0x07FFFFFF80000000  //!< RegionKey binary mask for START POS [ 00000111 11111111 11111111 11111111 10000000 00000000 00000000 00000000 ]
#define RKMASK_ENDPOS    0x000000007FFFFFF8  //!< RegionKey binary mask for END POS   [ 00000000 00000000 00000000 00000000 01111111 11111111 11111111 11111000 ]
#define RKMASK_STRAND    0x0000000000000006  //!< RegionKey binary mask for STRAND    [ 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000110 ]
#define RKMASK_NOPOS     0xF800000000000007  //!< RegionKey binary mask WITHOUT POS   [ 11111000 00000000 00000000 00000000 00000000 00000000 00000000 00000111 ]
#define RKSHIFT_CHROM     59 //!< CHROM LSB position from the VariantKey LSB
#define RKSHIFT_STARTPOS  31 //!< START POS LSB position from the VariantKey LSB
#define RKSHIFT_ENDPOS     3 //!< END POS LSB position from the VariantKey LSB
#define RKSHIFT_STRAND     1 //!< STRAND LSB position from the VariantKey LSB

#define RK_CHROM    ((rk & RKMASK_CHROM) >> RKSHIFT_CHROM)       //!< Extract the CHROM code from RegionKey.
#define RK_STARTPOS ((rk & RKMASK_STARTPOS) >> RKSHIFT_STARTPOS) //!< Extract the START POS code from RegionKey.
#define RK_ENDPOS   ((rk & RKMASK_ENDPOS) >> RKSHIFT_ENDPOS)     //!< Extract the END POS code from RegionKey.
#define RK_STRAND   ((rk & RKMASK_STRAND) >> RKSHIFT_STRAND)     //!< Extract the STRAND from RegionKey.

/**
 * @brief The numerically encoded RegionKey components.
 */
typedef struct regionkey_t
{
    uint8_t chrom;     //!< Chromosome encoded number (only the LSB 5 bit are used)
    uint32_t startpos; //!< Region start position (zero based)
    uint32_t endpos;   //!< Region end position (pos_start + region_length)
    uint8_t strand;    //!< Encoded region strand direction (0 > 0, 1 > +1, 2 > -1).
} regionkey_t;

/**
 * @brief The decoded components of a RegionKey.
 */
typedef struct regionkey_rev_t
{
    char chrom[3];     //!< Chromosome.
    uint32_t startpos; //!< Region start position (zero based)
    uint32_t endpos;   //!< Region end position (pos_start + region_length)
    int8_t strand;     //!< Region strand direction (-1, 0, +1)
} regionkey_rev_t;

/**
 * @brief Encodes a strand direction: -1 to 2, 0 to 0, +1 to 1.
 *
 * @param strand     Strand direction (-1, 0, +1).
 *
 * @return The strand code.
 */
static inline uint8_t encode_region_strand(int8_t strand)
{
    static const uint8_t map[] = {2, 0, 1, 0};
    return map[((uint8_t)(++strand) & 3)];
}

/**
 * @brief Decodes a strand code: 0 to 0, 1 to +1, 2 to -1.
 *
 * @param strand     Strand code.
 *
 * @return The strand direction.
 */
static inline int8_t decode_region_strand(uint8_t strand)
{
    static const int8_t map[] = {0, 1, -1, 0};
    return map[(strand & 3)];
}

/**
 * @brief Assembles a RegionKey from its pre-encoded components.
 *
 * @param chrom      Encoded chromosome (see encode_chrom).
 * @param startpos   Start position (zero based).
 * @param endpos     End position (startpos + region length).
 * @param strand     Encoded strand direction (see encode_region_strand).
 *
 * @return RegionKey 64 bit code.
 */
static inline uint64_t encode_regionkey(uint8_t chrom, uint32_t startpos, uint32_t endpos, uint8_t strand)
{
    return (((uint64_t)chrom << RKSHIFT_CHROM) | ((uint64_t)startpos << RKSHIFT_STARTPOS) | ((uint64_t)endpos << RKSHIFT_ENDPOS) | ((uint64_t)strand << RKSHIFT_STRAND));
}

/**
 * @brief Extracts the CHROM code from a RegionKey.
 *
 * @param rk RegionKey code.
 *
 * @return CHROM code.
 */
static inline uint8_t extract_regionkey_chrom(uint64_t rk)
{
    return (uint8_t)RK_CHROM;
}

/**
 * @brief Extracts the START POS value from a RegionKey.
 *
 * @param rk RegionKey code.
 *
 * @return START POS.
 */
static inline uint32_t extract_regionkey_startpos(uint64_t rk)
{
    return (uint32_t)RK_STARTPOS;
}

/**
 * @brief Extracts the END POS value from a RegionKey.
 *
 * @param rk RegionKey code.
 *
 * @return END POS.
 */
static inline uint32_t extract_regionkey_endpos(uint64_t rk)
{
    return (uint32_t)RK_ENDPOS;
}

/**
 * @brief Extracts the STRAND code from a RegionKey.
 *
 * @param rk RegionKey code.
 *
 * @return STRAND code.
 */
static inline uint8_t extract_regionkey_strand(uint64_t rk)
{
    return (uint8_t)RK_STRAND;
}

/**
 * @brief Splits a RegionKey into its encoded components.
 *
 * @param code RegionKey code.
 * @param rk   Structure containing the return values.
 */
static inline void decode_regionkey(uint64_t code, regionkey_t *rk)
{
    rk->chrom = extract_regionkey_chrom(code);
    rk->startpos = extract_regionkey_startpos(code);
    rk->endpos = extract_regionkey_endpos(code);
    rk->strand = extract_regionkey_strand(code);
}

/**
 * @brief Reverses a RegionKey into its decoded components.
 *
 * @param rk       RegionKey code.
 * @param rev      Structure containing the return values.
 */
static inline void reverse_regionkey(uint64_t rk, regionkey_rev_t *rev)
{
    decode_chrom(extract_regionkey_chrom(rk), rev->chrom);
    rev->startpos = extract_regionkey_startpos(rk);
    rev->endpos = extract_regionkey_endpos(rk);
    rev->strand = decode_region_strand(extract_regionkey_strand(rk));
}

/**
 * @brief Returns a RegionKey for the given CHROM, START POS (0-based), END POS and STRAND.
 *
 * @param chrom      Chromosome identifier, no white-space or leading zeros permitted.
 * @param sizechrom  Length of the chrom string, excluding the terminating null byte.
 * @param startpos   Start position (zero based).
 * @param endpos     End position (startpos + region length).
 * @param strand     Strand direction (-1, 0, +1).
 *
 * @return RegionKey 64 bit code.
 */
static inline uint64_t regionkey(const char *chrom, size_t sizechrom, uint32_t startpos, uint32_t endpos, int8_t strand)
{
    return encode_regionkey(encode_chrom(chrom, sizechrom), startpos, endpos, encode_region_strand(strand));
}

/**
 * @brief Extends a RegionKey region by a fixed amount at both ends.
 *
 * The result is clamped to the range from 0 to RK_MAX_POS.
 *
 * @param rk   RegionKey code.
 * @param size Amount to extend the region by.
 *
 * @return The extended RegionKey 64 bit code.
 */
static inline uint64_t extend_regionkey(uint64_t rk, uint32_t size)
{
    uint64_t startpos = RK_STARTPOS;
    uint64_t endpos = RK_ENDPOS;
    startpos = ((size >= startpos) ? 0 : (startpos - size));
    endpos = (((RK_MAX_POS - endpos) <= size) ? RK_MAX_POS : (endpos + size));
    return ((rk & RKMASK_NOPOS) | (startpos << RKSHIFT_STARTPOS) | (endpos << RKSHIFT_ENDPOS));
}

/**
 * @brief Writes a RegionKey as a 16 character hexadecimal string.
 *
 * @param rk    RegionKey code.
 * @param str   String buffer to be returned (it must be sized 17 bytes at least).
 *
 * @return The number of characters written, excluding the terminating null byte.
 */
static inline size_t regionkey_hex(uint64_t rk, char *str)
{
    return hex_uint64_t(rk, str);
}

/**
 * @brief Parses a 16 character hexadecimal string into a RegionKey.
 *
 * @param rs    RegionKey hexadecimal string (it must contain 16 hexadecimal characters).
 *
 * @return RegionKey 64 bit code.
 */
static inline uint64_t parse_regionkey_hex(const char *rs)
{
    return parse_hex_uint64_t(rs);
}

/**
 * @brief Returns the CHROM and START POS section of a RegionKey.
 *
 * @param rk RegionKey code.
 *
 * @return CHROM + START POS.
 */
static inline uint64_t get_regionkey_chrom_startpos(uint64_t rk)
{
    return (rk >> RKSHIFT_STARTPOS);
}

/**
 * @brief Returns the CHROM and END POS of a RegionKey.
 *
 * @param rk RegionKey code.
 *
 * @return CHROM + END POS.
 */
static inline uint64_t get_regionkey_chrom_endpos(uint64_t rk)
{
    return (((rk & RKMASK_CHROM) >> RKSHIFT_STARTPOS) | extract_regionkey_endpos(rk));
}

/**
 * @brief Checks whether two regions overlap.
 *
 * @param a_chrom     Region A chromosome code.
 * @param a_startpos  Region A start position.
 * @param a_endpos    Region A end position (startpos + region length).
 * @param b_chrom     Region B chromosome code.
 * @param b_startpos  Region B start position.
 * @param b_endpos    Region B end position (startpos + region length).
 *
 * @return 1 if the regions overlap, 0 otherwise.
 */
static inline uint8_t are_overlapping_regions(uint8_t a_chrom, uint32_t a_startpos, uint32_t a_endpos, uint8_t b_chrom, uint32_t b_startpos, uint32_t b_endpos)
{
    return (uint8_t)((a_chrom == b_chrom) && (a_startpos < b_endpos) && (a_endpos > b_startpos));
}

/**
 * @brief Checks whether a region and a RegionKey overlap.
 *
 * @param chrom     Region chromosome code.
 * @param startpos  Region start position.
 * @param endpos    Region end position (startpos + region length).
 * @param rk        RegionKey of the other region.
 *
 * @return 1 if the regions overlap, 0 otherwise.
 */
static inline uint8_t are_overlapping_region_regionkey(uint8_t chrom, uint32_t startpos, uint32_t endpos, uint64_t rk)
{
    return (uint8_t)((chrom == extract_regionkey_chrom(rk)) && (startpos < extract_regionkey_endpos(rk)) && (endpos > extract_regionkey_startpos(rk)));
}

/**
 * @brief Checks whether two RegionKeys overlap.
 *
 * @param rka        RegionKey A.
 * @param rkb        RegionKey B.
 *
 * @return 1 if the regions overlap, 0 otherwise.
 */
static inline uint8_t are_overlapping_regionkeys(uint64_t rka, uint64_t rkb)
{
    return (uint8_t)((extract_regionkey_chrom(rka) == extract_regionkey_chrom(rkb)) && (extract_regionkey_startpos(rka) < extract_regionkey_endpos(rkb)) && (extract_regionkey_endpos(rka) > extract_regionkey_startpos(rkb)));
}

/**
 * @brief Checks whether a VariantKey and a RegionKey overlap.
 *
 * @param nvc   Structure containing the pointers to the NRVK memory mapped file columns.
 * @param vk    VariantKey code.
 * @param rk    RegionKey code.
 *
 * @return 1 if the regions overlap, 0 otherwise.
 */
static inline uint8_t are_overlapping_variantkey_regionkey(nrvk_cols_t nvc, uint64_t vk, uint64_t rk)
{
    return (uint8_t)((extract_variantkey_chrom(vk) == extract_regionkey_chrom(rk)) && (extract_variantkey_pos(vk) < extract_regionkey_endpos(rk)) && (get_variantkey_endpos(nvc, vk) > extract_regionkey_startpos(rk)));
}

/**
 * @brief Converts a VariantKey into a RegionKey.
 *
 * The resulting region spans from the variant position to its end position and
 * carries no strand.
 *
 * @param nvc   Structure containing the pointers to the NRVK memory mapped file columns.
 * @param vk    VariantKey code.
 *
 * @return RegionKey 64 bit code.
 */
static inline uint64_t variantkey_to_regionkey(nrvk_cols_t nvc, uint64_t vk)
{
    return ((vk & VKMASK_CHROMPOS) | ((uint64_t)get_variantkey_endpos(nvc, vk) << RKSHIFT_ENDPOS));
}

#endif  // VARIANTKEY_REGIONKEY_H
