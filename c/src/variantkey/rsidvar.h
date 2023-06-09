// VariantKey
//
// rsidvar.h
//
// @category   Libraries
// @author     Nicola Asuni <info@tecnick.com>
// @link       https://github.com/tecnickcom/variantkey
// @license    MIT [LICENSE](https://raw.githubusercontent.com/tecnickcom/variantkey/main/LICENSE)

/**
 * @file rsidvar.h
 * @brief Functions to read VariantKey-rsID binary files.
 *
 * The functions provided here allows fast search for rsID and VariantKey values from binary files
 * made of adjacent constant-length binary blocks sorted in ascending order.
 *
 * rsvk.bin:
 * Lookup table to retrieve VariantKey from rsID.
 * This binary file can be generated by the `resources/tools/rsvk.sh' script from a TSV file.
 * This can also be in *Apache Arrow File* format with a single *RecordBatch*, or *Feather* format.
 * The first column must contain the rsID sorted in ascending order.
 *
 * vkrs.bin:
 * Lookup table to retrieve rsID from VariantKey.
 * This binary file can be generated by the `resources/tools/vkrs.sh' script from a TSV file.
 * This can also be in *Apache Arrow File* format with a single *RecordBatch*, or *Feather* format.
 * The first column must contain the VariantKey sorted in ascending order.
 */

#ifndef VARIANTKEY_RSIDVAR_H
#define VARIANTKEY_RSIDVAR_H

#include "binsearch.h"
#include "variantkey.h"

/**
 * Struct containing the RSVK or VKRS memory mapped file column info.
 */
typedef struct rsidvar_cols_t
{
    const uint64_t *vk;  //!< Pointer to the VariantKey column.
    const uint32_t *rs;  //!< Pointer to the rsID column.
    uint64_t nrows;      //!< Number of rows.
} rsidvar_cols_t;

/**
 * Memory map the VKRS binary file.
 *
 * @param file  Path to the file to map.
 * @param mf    Structure containing the memory mapped file.
 * @param cvr   Structure containing the pointers to the VKRS memory mapped file columns.
 *
 * @return Returns the memory-mapped file descriptors.
 */
static inline void mmap_vkrs_file(const char *file, mmfile_t *mf, rsidvar_cols_t *cvr)
{
    mmap_binfile(file, mf);
    cvr->vk = (const uint64_t *)(mf->src + mf->index[0]);
    cvr->rs = (const uint32_t *)(mf->src + mf->index[1]);
    cvr->nrows = mf->nrows;
}

/**
 * Memory map the RSVK binary file.
 *
 * @param file  Path to the file to map.
 * @param mf    Structure containing the memory mapped file.
 * @param crv   Structure containing the pointers to the RSVK memory mapped file columns.
 *
 * @return Returns the memory-mapped file descriptors.
 */
static inline void mmap_rsvk_file(const char *file, mmfile_t *mf, rsidvar_cols_t *crv)
{
    mmap_binfile(file, mf);
    crv->rs = (const uint32_t *)(mf->src + mf->index[0]);
    crv->vk = (const uint64_t *)(mf->src + mf->index[1]);
    crv->nrows = mf->nrows;
}

/**
 * Search for the specified rsID and returns the first occurrence of VariantKey in the RV file.
 *
 * @param crv       Structure containing the pointers to the RSVK memory mapped file columns (rsvk.bin).
 * @param first     Pointer to the first element of the range to search (min value = 0).
 *                  This will hold the position of the first record found.
 * @param last      Element (up to but not including) where to end the search (max value = nitems).
 * @param rsid      rsID to search.
 *
 * @return VariantKey data or zero data if not found
 */
static inline uint64_t find_rv_variantkey_by_rsid(rsidvar_cols_t crv, uint64_t *first, uint64_t last, uint32_t rsid)
{
    uint64_t max = last;
    uint64_t found = col_find_first_uint32_t(crv.rs, first, &max, rsid);
    if (found >= last)
    {
        return 0;
    }
    *first = found;
    return *(crv.vk + found);
}

/**
 * Get the next VariantKey for the specified rsID in the RV file.
 * This function should be used after find_rv_variantkey_by_rsid.
 * This function can be called in a loop to get all VariantKeys that are associated with the same rsID (if any).
 *
 * @param crv       Structure containing the pointers to the RSVK memory mapped file columns (rsvk.bin).
 * @param pos       Pointer to the current item. This will hold the position of the next record.
 * @param last      Element (up to but not including) where to end the search (max value = nitems).
 * @param rsid      rsID to search.
 *
 * @return VariantKey data or zero data if not found
 */
static inline uint64_t get_next_rv_variantkey_by_rsid(rsidvar_cols_t crv, uint64_t *pos, uint64_t last, uint32_t rsid)
{
    if (col_has_next_uint32_t(crv.rs, pos, last, rsid))
    {
        return *(crv.vk + *pos);
    }
    return 0;
}

/**
 * Search for the specified VariantKey and returns the first occurrence of rsID in the VR file.
 *
 * @param cvr       Structure containing the pointers to the VKRS memory mapped file columns (vkrs.bin).
 * @param first     Pointer to the first element of the range to search (min value = 0).
 *                  This will hold the position of the first record found.
 * @param last      Element (up to but not including) where to end the search (max value = nitems).
 * @param vk        VariantKey.
 *
 * @return rsID or 0 if not found
 */
static inline uint32_t find_vr_rsid_by_variantkey(rsidvar_cols_t cvr, uint64_t *first, uint64_t last, uint64_t vk)
{
    uint64_t max = last;
    uint64_t found = col_find_first_uint64_t(cvr.vk, first, &max, vk);
    if (found >= last)
    {
        return 0; // not found
    }
    *first = found;
    return *(cvr.rs + found);
}

/**
 * Get the next rsID for the specified VariantKey in the VR file.
 * This function should be used after find_vr_rsid_by_variantkey.
 * This function can be called in a loop to get all rsIDs that are associated with the same VariantKey (if any).
 *
 * @param cvr       Structure containing the pointers to the VKRS memory mapped file columns (vkrs.bin).
 * @param pos       Pointer to the current item. This will hold the position of the next record.
 * @param last      Element (up to but not including) where to end the search (max value = nitems).
 * @param vk        VariantKey.
 *
 * @return rsID data or zero data if not found
 */
static inline uint32_t get_next_vr_rsid_by_variantkey(rsidvar_cols_t cvr, uint64_t *pos, uint64_t last, uint64_t vk)
{
    if (col_has_next_uint64_t(cvr.vk, pos, last, vk))
    {
        return *(cvr.rs + *pos);
    }
    return 0;
}

/**
 * Search for the specified CHROM-POS range and returns the first occurrence of rsID in the VR file.
 *
 * @param cvr       Structure containing the pointers to the VKRS memory mapped file columns (vkrs.bin).
 * @param first     Pointer to the first element of the range to search (min value = 0).
 * @param last      Pointer to the Element (up to but not including) where to end the search (max value = nitems).
 * @param chrom     Chromosome encoded number.
 * @param pos_min   Start reference position, with the first base having position 0.
 * @param pos_max   End reference position, with the first base having position 0.
 *
 * @return rsID
 */
static inline uint32_t find_vr_chrompos_range(rsidvar_cols_t cvr, uint64_t *first, uint64_t *last, uint8_t chrom, uint32_t pos_min, uint32_t pos_max)
{
    uint64_t ckey = ((uint64_t)chrom << 59);
    uint64_t min = *first;
    uint64_t max = *last;
    *first = col_find_first_sub_uint64_t(cvr.vk, 0, 32, &min, &max, (ckey | ((uint64_t)pos_min << 31)) >> 31);
    if (*first >= *last)
    {
        return 0;
    }
    min = *first;
    max = *last;
    uint64_t end = col_find_last_sub_uint64_t(cvr.vk, 0, 32, &min, &max, (ckey | ((uint64_t)pos_max << 31)) >> 31);
    if (end < *last)
    {
        ++end;
    }
    *last = end;
    return *(cvr.rs + *first);
}

#endif  // VARIANTKEY_RSIDVAR_H
