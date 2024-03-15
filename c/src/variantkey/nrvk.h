// VariantKey
//
// nrvk.h
//
// @category   Libraries
// @author     Nicola Asuni <info@tecnick.com>
// @link       https://github.com/tecnickcom/variantkey
// @license    MIT [LICENSE](https://raw.githubusercontent.com/tecnickcom/variantkey/main/LICENSE)

/**
 * @file nrvk.h
 * @brief Functions to retrieve REF and ALT values by VariantKey from binary data file.
 *
 * The functions provided here allows to retrieve the REF and ALT strings for a
 * given VariantKey from a binary file.
 *
 * The input binary files can be generated from a normalized VCF file using the `resources/tools/vkhexbin.sh`.
 * The VCF file can be normalized using the `resources/tools/vcfnorm.sh` script.
 *
 * The binary file can be generated by the `resources/tools/nrvk.sh' script from a TSV file with the following format:
 *
 *     [16 BYTE VARIANTKEY HEX]\t[REF STRING]\t[ALT STRING]\n...
 *
 * for example:
 *
 *     b800c35bbcece603	AAAAAAAAGG	AG
 *     1800c351f61f65d3	A	AAGAAAGAAAG
 */

#ifndef VARIANTKEY_NRVK_H
#define VARIANTKEY_NRVK_H

#include <stdio.h>
#include <string.h>
#include "binsearch.h"
#include "variantkey.h"

#ifndef ALLELE_MAXSIZE
#define ALLELE_MAXSIZE 256 //!< Maximum allele length.
#endif

/**
 * VariantKey decoded struct
 */
typedef struct variantkey_rev_t
{
    char chrom[3];             //!< Chromosome.
    uint32_t pos;              //!< Reference position, with the first base having position 0.
    char ref[ALLELE_MAXSIZE];  //!< Reference allele
    char alt[ALLELE_MAXSIZE];  //!< Alternate allele
    size_t sizeref;            //!< Length of reference allele
    size_t sizealt;            //!< Length of alternate allele
} variantkey_rev_t;

/**
 * Struct containing the NRVK memory mapped file column info.
 */
typedef struct nrvk_cols_t
{
    const uint64_t *vk;      //!< Pointer to the VariantKey column.
    const uint64_t *offset;  //!< Pointer to the Offset column.
    const uint8_t  *data;    //!< Pointer to the Data column.
    uint64_t nrows;          //!< Number of rows.
} nrvk_cols_t;

/**
 * Memory map the NRVK binary file.
 *
 * @param file  Path to the file to map.
 * @param mf    Structure containing the memory mapped file.
 * @param nvc   Structure containing the pointers to the memory mapped file columns.
 *
 * @return Returns the memory-mapped file descriptors.
 */
static inline void mmap_nrvk_file(const char *file, mmfile_t *mf, nrvk_cols_t *nvc)
{
    mmap_binfile(file, mf);
    nvc->vk = (const uint64_t *)(mf->src + mf->index[0]);
    nvc->offset = (const uint64_t *)(mf->src + mf->index[1]);
    nvc->data = (const uint8_t *)(mf->src + mf->index[2]);
    nvc->nrows = mf->nrows;
}

static inline size_t get_nrvk_ref_alt_by_pos(nrvk_cols_t nvc, uint64_t pos, char *ref, size_t *sizeref, char *alt, size_t *sizealt)
{
    if (pos >= nvc.nrows)
    {
        return 0; // not found
    }
    const uint8_t *data = (nvc.data + *(nvc.offset + pos));
    *sizeref = (size_t)(*(data++));
    *sizealt = (size_t)(*(data++));
    memcpy(ref, data, *sizeref);
    ref[*sizeref] = 0;
    memcpy(alt, (data + *sizeref), *sizealt);
    alt[*sizealt] = 0;
    return (*sizeref + *sizealt);
}

/**
 * Retrieve the REF and ALT strings for the specified VariantKey.
 *
 * @param nvc      Structure containing the pointers to the memory mapped file columns.
 * @param vk       VariantKey to search.
 * @param ref      REF string buffer to be returned.
 * @param sizeref  Pointer to the size of the ref buffer, excluding the terminating null byte.
 *                 This will contain the final ref size.
 * @param alt      ALT string buffer to be returned.
 * @param sizealt  Pointer to the size of the alt buffer, excluding the terminating null byte.
 *                 This will contain the final alt size.
 *
 * @return REF+ALT length or 0 if the VariantKey is not found.
 */
static inline size_t find_ref_alt_by_variantkey(nrvk_cols_t nvc, uint64_t vk, char *ref, size_t *sizeref, char *alt, size_t *sizealt)
{
    uint64_t first = 0;
    uint64_t max = nvc.nrows;
    uint64_t found = col_find_first_uint64_t(nvc.vk, &first, &max, vk);
    return get_nrvk_ref_alt_by_pos(nvc, found, ref, sizeref, alt, sizealt);
}

/**
 * Reverse a VariantKey code and returns the normalized components as variantkey_rev_t structure.
 *
 * @param nvc      Structure containing the pointers to the memory mapped file columns.
 * @param vk       VariantKey code.
 * @param rev      Structure containing the return values.
 *
 * @return A variantkey_rev_t structure.
 */
static inline size_t reverse_variantkey(nrvk_cols_t nvc, uint64_t vk, variantkey_rev_t *rev)
{
    decode_chrom(extract_variantkey_chrom(vk), rev->chrom);
    rev->pos = extract_variantkey_pos(vk);
    size_t len = decode_refalt(extract_variantkey_refalt(vk), rev->ref, &rev->sizeref, rev->alt, &rev->sizealt);
    if ((len == 0) && (nvc.nrows > 0))
    {
        len = find_ref_alt_by_variantkey(nvc, vk, rev->ref, &rev->sizeref, rev->alt, &rev->sizealt);
    }
    return len;
}

/**
 * Retrieve the REF length for the specified VariantKey.
 *
 * @param nvc      Structure containing the pointers to the memory mapped file columns.
 * @param vk       VariantKey.
 *
 * @return REF length or 0 if the VariantKey is not reversible and not found.
 */
static inline size_t get_variantkey_ref_length(nrvk_cols_t nvc, uint64_t vk)
{
    if ((vk & 0x1) == 0) // check last bit for reversible encoding
    {
        return ((vk & 0x0000000078000000) >> 27); // [00000000 00000000 00000000 00000000 01111000 00000000 00000000 00000000]
    }
    uint64_t first = 0;
    uint64_t max = nvc.nrows;
    uint64_t found = col_find_first_uint64_t(nvc.vk, &first, &max, vk);
    if (found >= nvc.nrows)
    {
        return 0; // not found
    }
    return (size_t)(*(nvc.data + *(nvc.offset + found)));
}

/**
 * Get the VariantKey end position (POS + REF length).
 *
 * @param nvc      Structure containing the pointers to the memory mapped file columns.
 * @param vk       VariantKey.
 *
 * @return Variant end position (POS + REF length).
 */
static inline uint32_t get_variantkey_endpos(nrvk_cols_t nvc, uint64_t vk)
{
    return (extract_variantkey_pos(vk) + (uint32_t)get_variantkey_ref_length(nvc, vk));
}

/** @brief Get the CHROM + START POS encoding from VariantKey.
 *
 * @param vk VariantKey code.
 *
 * @return CHROM + START POS.
 */
static inline uint64_t get_variantkey_chrom_startpos(uint64_t vk)
{
    return (vk >> VKSHIFT_POS);
}

/** @brief Get the CHROM + END POS encoding from VariantKey.
 *
 * @param nvc   Structure containing the pointers to the memory mapped file columns.
 * @param vk    VariantKey code.
 *
 * @return CHROM + END POS.
 */
static inline uint64_t get_variantkey_chrom_endpos(nrvk_cols_t nvc, uint64_t vk)
{
    return (((vk & VKMASK_CHROM) >> VKSHIFT_POS) | (uint64_t)get_variantkey_endpos(nvc, vk));
}

/**
 * Convert a vrnr.bin file to a simple TSV.
 * For the reverse operation see the resources/tools/nrvk.sh script.
 *
 * @param nvc      Structure containing the pointers to the memory mapped file columns.
 * @param tsvfile  Output tsv file name. NOTE: existing files will be replaced.
 *
 * @return Number of written bytes or 0 in case of error.
 */
static inline size_t nrvk_bin_to_tsv(nrvk_cols_t nvc, const char *tsvfile)
{
    FILE *fp = 0;
    size_t sizeref = 0, sizealt = 0, len = 0;
    char ref[ALLELE_MAXSIZE];
    char alt[ALLELE_MAXSIZE];
    uint64_t i = 0;
    fp = fopen(tsvfile, "we");
    if (fp == NULL)
    {
        return 0;
    }
    for (i = 0; i < nvc.nrows; i++)
    {
        len += (get_nrvk_ref_alt_by_pos(nvc, i, ref, &sizeref, alt, &sizealt) + 19);
        (void) fprintf(fp, "%016" PRIx64 "\t%s\t%s\n", *nvc.vk++, ref, alt);
    }
    (void) fclose(fp);
    return len;
}

#endif  // VARIANTKEY_NRVK_H
