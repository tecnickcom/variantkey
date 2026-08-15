// VariantKey
//
// genoref.h
//
// @category   Libraries
// @author     Nicola Asuni <info@tecnick.com>
// @link       https://github.com/tecnickcom/variantkey
// @license    MIT [LICENSE](https://raw.githubusercontent.com/tecnickcom/variantkey/main/LICENSE)

/**
 * @file genoref.h
 * @brief Genome reference lookup and variant normalization.
 *
 * Reads genome reference sequences from a memory mapped binary version of a
 * genome reference FASTA file, and uses them to check and normalize variants.
 *
 * The binary file can be generated from a FASTA file with the
 * `resources/tools/fastabin.sh` script.
 *
 * The lookup functions take the memory mapped file as a `const mmfile_t *`
 * rather than by value: `mmfile_t` embeds `ctbytes[256]` and `index[256]`, so
 * it is over 2 KB and a copy per call would be required otherwise.
 */

#ifndef VARIANTKEY_GENOREF_H
#define VARIANTKEY_GENOREF_H

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "binsearch.h"
#include "variantkey.h"

#ifndef ALLELE_MAXSIZE
#define ALLELE_MAXSIZE 256 //!< Maximum allele length.
#endif

// Return codes for normalize_variant()
#define NORM_WRONGPOS   (-2) //!< Normalization: Invalid position.
#define NORM_INVALID    (-1) //!< Normalization: Invalid reference.
#define NORM_OK          (0) //!< Normalization: The reference allele perfectly match the genome reference.
#define NORM_VALID       (1) //!< Normalization: The reference allele is inconsistent with the genome reference (i.e. when contains nucleotide letters other than A, C, G and T).
#define NORM_SWAP   (1 << 1) //!< Normalization: The alleles have been swapped.
#define NORM_FLIP   (1 << 2) //!< Normalization: The alleles nucleotides have been flipped (each nucleotide have been replaced with its complement).
#define NORM_LEXT   (1 << 3) //!< Normalization: Alleles have been left extended.
#define NORM_RTRIM  (1 << 4) //!< Normalization: Alleles have been right trimmed.
#define NORM_LTRIM  (1 << 5) //!< Normalization: Alleles have been left trimmed.

#define GENOREF_MAXCHROM 25 //!< Highest chromosome code with a genome reference sequence.

/**
 * @brief Memory maps the genoref binary file.
 *
 * The column index is shifted by one so that index[chrom] is the offset of the
 * first byte of the sequence of the chromosome with code chrom, and index[26]
 * is the end of the last sequence.
 *
 * @param file  Path to the file to map.
 * @param mf    Structure containing the memory mapped file.
 */
static inline void mmap_genoref_file(const char *file, mmfile_t *mf)
{
    mmap_binfile(file, mf);
    mf->index[26] = mf->size;
    int i = 25;
    while (i > 0)
    {
        mf->index[i] = mf->index[(i - 1)];
        i--;
    }
    mf->ncols = 27;
}

/**
 * @brief Returns the uppercase version of a lowercase letter.
 *
 * Every character from 'a' upwards is transformed, so this is only safe for the
 * a to z range.
 *
 * @param c Character to uppercase.
 *
 * @return The uppercased character.
 */
static inline int aztoupper(int c)
{
    if (c >= 'a')
    {
        return (c ^ ('a' - 'A'));
    }
    return c;
}

/**
 * @brief Prepends a character to a null terminated string.
 *
 * The size is incremented in place. The buffer must have room for one more character.
 *
 * @param pre    Character to prepend.
 * @param string String to modify.
 * @param size   Pointer to the length of the string, excluding the terminating null byte.
 */
static inline void prepend_char(const char pre, char *string, size_t *size)
{
    memmove(string + 1, string, (*size + 1));
    string[0] = pre;
    (*size)++;
}

/**
 * @brief Returns the genome reference nucleotide at the given chromosome and position.
 *
 * @param mf      Pointer to the structure containing the memory mapped file.
 * @param chrom   Encoded chromosome number (see encode_chrom), up to GENOREF_MAXCHROM.
 * @param pos     Reference position, with the first base having position 0.
 *
 * @return The nucleotide letter, or 0 for an invalid chromosome or position.
 */
static inline char get_genoref_seq(const mmfile_t *mf, uint8_t chrom, uint32_t pos)
{
    if (chrom > GENOREF_MAXCHROM)
    {
        return 0; // invalid chromosome
    }
    uint64_t offset = (mf->index[chrom] + pos);
    if (offset >= mf->index[(chrom + 1)])
    {
        return 0; // invalid position
    }
    return  (char)*(mf->src + offset);
}

/**
 * @brief Checks a reference allele against the genome reference data.
 *
 * Degenerate base symbols are accepted when their base sets can overlap, as
 * listed in the table inside the function body.
 *
 * @param mf      Pointer to the structure containing the memory mapped file.
 * @param chrom   Encoded chromosome number (see encode_chrom), up to GENOREF_MAXCHROM.
 * @param pos     Reference position, with the first base having position 0.
 * @param ref     Reference allele. String containing a sequence of nucleotide letters.
 * @param sizeref Length of the ref string, excluding the terminating null byte.
 *
 * @return NORM_OK (0) if the allele matches the genome reference,
 *         NORM_VALID (1) if it is compatible but contains letters other than A, C, G and T,
 *         NORM_INVALID (-1) if it does not match,
 *         NORM_WRONGPOS (-2) if the chromosome is invalid or the allele extends beyond the sequence.
 */
static inline int check_reference(const mmfile_t *mf, uint8_t chrom, uint32_t pos, const char *ref, size_t sizeref)
{
    if (chrom > GENOREF_MAXCHROM)
    {
        return NORM_WRONGPOS;
    }
    uint64_t offset = (mf->index[chrom] + pos);
    if ((offset + sizeref) > mf->index[(chrom + 1)])
    {
        return NORM_WRONGPOS;
    }
    size_t i = 0;
    int ret = 0; // return value
    for (i = 0; i < sizeref; i++)
    {
        char uref = (char) aztoupper(ref[i]);
        char gref = (char) mf->src[(offset + i)];
        if (uref == gref)
        {
            continue;
        }
        /*
            Abbreviation codes for degenerate bases

            Cornish-Bowden A.
            Nomenclature for incompletely specified bases in nucleic acid sequences: recommendations 1984.
            Nucleic Acids Research. 1985;13(9):3021-3030.

            SYMBOL | DESCRIPTION                   | BASES   | COMPLEMENT | SET
            -------+-------------------------------+---------+------------+-----
               A   | Adenine                       | A       |  T         |  1
               C   | Cytosine                      |   C     |  G         |  2
               G   | Guanine                       |     G   |  C         |  4
               T   | Thymine                       |       T |  A         |  8
               W   | Weak                          | A     T |  W         |  9
               S   | Strong                        |   C G   |  S         |  6
               M   | aMino                         | A C     |  K         |  3
               K   | Keto                          |     G T |  M         | 12
               R   | puRine                        | A   G   |  Y         |  5
               Y   | pYrimidine                    |   C   T |  R         | 10
               B   | not A (B comes after A)       |   C G T |  V         | 14
               D   | not C (D comes after C)       | A   G T |  H         | 13
               H   | not G (H comes after G)       | A C   T |  D         | 11
               V   | not T (V comes after T and U) | A C G   |  B         |  7
               N   | aNy base (not a gap)          | A C G T |  N         | 15
            -------+-------------------------------+---------+------------+-----

            Two symbols are compatible when their base sets intersect. The SET
            column above encodes each set as a 4 bit mask (A=1, C=2, G=4, T=8),
            so the whole test is a table lookup and a bitwise AND.

            Any symbol outside the fifteen listed above maps to the empty set and
            intersects with nothing. There is no U row: U is treated as an
            ordinary letter.
        */
        // *INDENT-OFF*
        static const uint8_t baseset[256] =
        {
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
            /*   A   B   C   D   E   F   G   H   I   J   K   L   M   N   O   P   Q   R   S   T   U   V   W   X   Y   Z */
             0,  1, 14,  2, 13,  0,  0,  4, 11,  0,  0, 12,  0,  3, 15,  0,  0,  0,  5,  6,  8,  0,  7,  9,  0, 10,  0,  0,  0,  0,  0,  0,
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        };
        // *INDENT-ON*
        if ((baseset[(uint8_t)uref] & baseset[(uint8_t)gref]) != 0)
        {
            ret = NORM_VALID; // valid but not consistent
            continue;
        }
        return NORM_INVALID; // invalid reference
    }
    return ret; // sequence OK
}

/**
 * @brief Replaces each nucleotide of an allele with its complement.
 *
 * The result is always uppercase and null terminated. Degenerate base symbols
 * are supported; any other character becomes '0'.
 *
 * @param allele  Allele string to modify.
 * @param size    Length of the allele string, excluding the terminating null byte.
 */
static inline void flip_allele(char *allele, size_t size)
{
    /*
      Byte map for allele flipping (complement):

      A > T
      T > A
      C > G
      G > C
      M > K
      K > M
      R > Y
      Y > R
      B > V
      V > B
      D > H
      H > D
    */
    static const char map[] = "00000000000000000000000000000000"
                              "00000000000000000123456789000000"
                              /*ABCDEFGHIJKLMNOPQRSTUVWXYZ*/
                              "0TVGHEFCDIJMLKNOPQYSAUBWXRZ00000"
                              /*abcdefghijklmnopqrstuvwxyz*/
                              "0TVGHEFCDIJMLKNOPQYSAUBWXRZ00000"
                              "00000000000000000000000000000000"
                              "00000000000000000000000000000000"
                              "00000000000000000000000000000000"
                              "00000000000000000000000000000000";
    size_t i = 0;
    for (i = 0; i < size; i++)
    {
        allele[i] = map[((uint8_t)allele[i])];
    }
    allele[size] = 0;
}

/**
 * @brief Swaps two sizes.
 *
 * @param first  Pointer to the first size.
 * @param second Pointer to the second size.
 */
static inline void swap_sizes(size_t *first, size_t *second)
{
    size_t tmp = *first;
    *first = *second;
    *second = tmp;
}

/**
 * @brief Swaps two alleles and their sizes.
 *
 * Both buffers must be ALLELE_MAXSIZE bytes.
 *
 * @param first       First allele string.
 * @param sizefirst   Pointer to the length of the first allele string, excluding the terminating null byte.
 * @param second      Second allele string.
 * @param sizesecond  Pointer to the length of the second allele string, excluding the terminating null byte.
 */
static inline void swap_alleles(char *first, size_t *sizefirst, char *second, size_t *sizesecond)
{
    char tmp[ALLELE_MAXSIZE];
    memcpy(tmp, first, *sizefirst);
    memcpy(first, second, *sizesecond);
    memcpy(second, tmp, *sizefirst);
    swap_sizes(sizefirst, sizesecond);
    first[*sizefirst] = 0;
    second[*sizesecond] = 0;
}

/**
 * @brief Normalizes a variant in place against the genome reference.
 *
 * Swaps and flips the alleles when that makes the reference match, then applies
 * the algorithm described at https://genome.sph.umich.edu/wiki/Variant_Normalization
 * The position, the alleles and their sizes are updated in place. Both allele
 * buffers must be ALLELE_MAXSIZE bytes.
 *
 * @param mf         Pointer to the structure containing the memory mapped file.
 * @param chrom      Encoded chromosome number, up to GENOREF_MAXCHROM.
 * @param pos        Pointer to the reference position, with the first base having position 0.
 * @param ref        Reference allele. String containing a sequence of nucleotide letters.
 * @param sizeref    Pointer to the length of the ref string, excluding the terminating null byte.
 * @param alt        Alternate non-reference allele string.
 * @param sizealt    Pointer to the length of the alt string, excluding the terminating null byte.
 *
 * @return NORM_WRONGPOS (-2) or NORM_INVALID (-1) on error, otherwise a bitmask
 *         of the applied transformations:
 *         - bit 0 (NORM_VALID) : the reference allele contains letters other than A, C, G and T.
 *         - bit 1 (NORM_SWAP)  : the alleles have been swapped.
 *         - bit 2 (NORM_FLIP)  : the alleles have been replaced with their complement.
 *         - bit 3 (NORM_LEXT)  : the alleles have been left extended.
 *         - bit 4 (NORM_RTRIM) : the alleles have been right trimmed.
 *         - bit 5 (NORM_LTRIM) : the alleles have been left trimmed.
 */
static inline int normalize_variant(const mmfile_t *mf, uint8_t chrom, uint32_t *pos, char *ref, size_t *sizeref, char *alt, size_t *sizealt)
{
    int status = check_reference(mf, chrom, *pos, ref, *sizeref);
    if (status == NORM_WRONGPOS)
    {
        return status; // invalid position
    }
    if (status < 0)
    {
        status = check_reference(mf, chrom, *pos, alt, *sizealt);
        if (status >= 0)
        {
            swap_alleles(ref, sizeref, alt, sizealt);
            status |= NORM_SWAP;
        }
        else
        {
            char fref[ALLELE_MAXSIZE];
            memcpy(fref, ref, *sizeref);
            flip_allele(fref, *sizeref);
            status = check_reference(mf, chrom, *pos, fref, *sizeref);
            if (status >= 0)
            {
                memcpy(ref, fref, *sizeref);
                ref[*sizeref] = 0;
                flip_allele(alt, *sizealt);
                status |= NORM_FLIP;
            }
            else
            {
                char falt[ALLELE_MAXSIZE];
                memcpy(falt, alt, *sizealt);
                flip_allele(falt, *sizealt);
                status = check_reference(mf, chrom, *pos, falt, *sizealt);
                if (status >= 0)
                {
                    memcpy(ref, falt, *sizealt);
                    ref[*sizealt] = 0;
                    memcpy(alt, fref, *sizeref);
                    alt[*sizeref] = 0;
                    swap_sizes(sizeref, sizealt);
                    status |= NORM_SWAP + NORM_FLIP;
                }
                else
                {
                    return status; // invalid reference
                }
            }
        }
    }
    if ((*sizealt == 1) && (*sizeref == 1))
    {
        return status; // SNP
    }
    while (1)
    {
        // left extend
        if (((*sizealt == 0) || (*sizeref == 0)) && (*pos > 0))
        {
            (*pos)--;
            char left = (char)mf->src[(mf->index[chrom] + *pos)];
            prepend_char(left, alt, sizealt);
            prepend_char(left, ref, sizeref);
            status |= NORM_LEXT;
        }
        else
        {
            // right trim
            if ((*sizealt > 1) && (*sizeref > 1) && (aztoupper(alt[(*sizealt - 1)]) == aztoupper(ref[(*sizeref - 1)])))
            {
                (*sizealt)--;
                (*sizeref)--;
                status |= NORM_RTRIM;
            }
            else
            {
                break;
            }
        }
    }
    // left trim
    size_t offset = 0;
    while (((offset + 1) < *sizealt) && ((offset + 1) < *sizeref) && (aztoupper(alt[offset]) == aztoupper(ref[offset])))
    {
        offset++;
    }
    if (offset > 0)
    {
        *pos += offset;
        *sizeref -= offset;
        *sizealt -= offset;
        memmove(ref, ref + offset, *sizeref);
        memmove(alt, alt + offset, *sizealt);
        status |= NORM_LTRIM;
    }
    ref[*sizeref] = 0;
    alt[*sizealt] = 0;
    return status;
}

/**
 * @brief Normalizes a variant and returns its VariantKey.
 *
 * The position, the alleles and their sizes are updated in place. Both allele
 * buffers must be ALLELE_MAXSIZE bytes.
 *
 * @param mf         Pointer to the structure containing the memory mapped file.
 * @param chrom      Chromosome identifier, no white-space or leading zeros permitted.
 * @param sizechrom  Length of the chrom string, excluding the terminating null byte.
 * @param pos        Pointer to the reference position.
 * @param posindex   Position index: 0 for 0-based, 1 for 1-based.
 * @param ref        Reference allele. Characters must be A-Z, a-z or *.
 * @param sizeref    Pointer to the length of the ref string, excluding the terminating null byte.
 * @param alt        Alternate non-reference allele. Characters must be A-Z, a-z or *.
 * @param sizealt    Pointer to the length of the alt string, excluding the terminating null byte.
 * @param ret        Pointer to the normalization return value (see normalize_variant).
 *
 * @return The normalized VariantKey 64 bit code.
 */
static inline uint64_t normalized_variantkey(const mmfile_t *mf, const char *chrom, size_t sizechrom, uint32_t *pos, uint8_t posindex, char *ref, size_t *sizeref, char *alt, size_t *sizealt, int *ret)
{
    uint8_t echrom = encode_chrom(chrom, sizechrom);
    (*pos) -= posindex;
    *ret = normalize_variant(mf, echrom, pos, ref, sizeref, alt, sizealt);
    return encode_variantkey(echrom, *pos, encode_refalt(ref, *sizeref, alt, *sizealt));
}

#endif  // VARIANTKEY_GENOREF_H
