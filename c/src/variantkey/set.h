// VariantKey
//
// set.h
//
// @category   Libraries
// @author     Nicola Asuni <info@tecnick.com>
// @link       https://github.com/tecnickcom/variantkey
// @license    MIT [LICENSE](https://raw.githubusercontent.com/tecnickcom/variantkey/main/LICENSE)

/**
 * @file set.h
 * @brief In-memory sorting and set operations on uint64_t arrays.
 */

#ifndef VARIANTKEY_SET_H
#define VARIANTKEY_SET_H

#include <inttypes.h>
#include <stdint.h>
#include <string.h>

// "restrict" is C99 only and these headers are also compiled as C++, so the
// keyword is used through a macro that expands to nothing where unavailable.
#ifndef VK_RESTRICT
#if defined(__cplusplus)
#define VK_RESTRICT
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
#define VK_RESTRICT restrict
#else
#define VK_RESTRICT
#endif
#endif

/**
 * @brief Number of bytes of a uint64_t, i.e. the number of radix sort passes.
 */
#define RADIX_SORT_PASSES 8

/**
 * @brief Builds the per-byte histograms of an array and turns them into offsets.
 *
 * Each of the RADIX_SORT_PASSES rows of cnt is the exclusive prefix sum of the
 * byte histogram for that byte position, i.e. the first output offset of each
 * bucket. The rows of the passes that can be skipped are left as plain counts.
 *
 * @param arr    First element of the array to scan.
 * @param nitems Number of elements in the array.
 * @param cnt    Bucket offsets to be returned.
 *
 * @return The bitwise OR of the differences between the elements and the first
 *         one: a byte of the result is zero exactly when that byte is identical
 *         in every element, so the corresponding pass can be skipped.
 */
static inline uint64_t radix_sort_count(const uint64_t *arr, uint32_t nitems, uint32_t cnt[RADIX_SORT_PASSES][256])
{
    uint64_t diff = 0;
    uint32_t i = 0;
    uint8_t b = 0;
    memset(cnt, 0, (sizeof(uint32_t) * RADIX_SORT_PASSES * 256));
    // The counting loop is unrolled over the eight byte positions.
    const uint64_t first = ((nitems > 0) ? arr[0] : 0);
    for (i = 0; i < nitems; i++)
    {
        uint64_t v = arr[i];
        diff |= (v ^ first);
        cnt[0][(v & 0xff)]++;
        cnt[1][((v >> 8) & 0xff)]++;
        cnt[2][((v >> 16) & 0xff)]++;
        cnt[3][((v >> 24) & 0xff)]++;
        cnt[4][((v >> 32) & 0xff)]++;
        cnt[5][((v >> 40) & 0xff)]++;
        cnt[6][((v >> 48) & 0xff)]++;
        cnt[7][((v >> 56) & 0xff)]++;
    }
    for (b = 0; b < RADIX_SORT_PASSES; b++)
    {
        if (((diff >> (b << 3)) & 0xff) == 0)
        {
            continue; // this byte is the same in every element: the pass is skipped
        }
        uint32_t offset = 0;
        for (i = 0; i < 256; i++)
        {
            uint32_t total = (offset + cnt[b][i]);
            cnt[b][i] = offset;
            offset = total;
        }
    }
    return diff;
}

/**
 * @brief Sorts an array of uint64_t values in ascending order with a radix sort.
 *
 * A pass whose byte is identical in every element is skipped. Skipping changes
 * the parity of the buffer ping-pong, so the result is copied back when it ends
 * up in the temporary buffer.
 *
 * @param arr    First element of the array to sort in place.
 * @param tmp    First element of a temporary array of nitems elements.
 * @param nitems Number of elements in the array.
 */
static inline void sort_uint64_t(uint64_t *VK_RESTRICT arr, uint64_t *VK_RESTRICT tmp, uint32_t nitems)
{
    uint32_t cnt[RADIX_SORT_PASSES][256];
    uint64_t diff = radix_sort_count(arr, nitems, cnt);
    uint64_t *src = arr;
    uint64_t *dst = tmp;
    uint8_t b = 0;
    for (b = 0; b < RADIX_SORT_PASSES; b++)
    {
        if (((diff >> (b << 3)) & 0xff) == 0)
        {
            continue; // this byte is the same in every element
        }
        uint32_t i = 0;
        for (i = 0; i < nitems; i++)
        {
            uint64_t v = src[i];
            dst[cnt[b][((v >> (b << 3)) & 0xff)]++] = v;
        }
        uint64_t *swap = src;
        src = dst;
        dst = swap;
    }
    if (src != arr)
    {
        memcpy(arr, src, (nitems * sizeof(uint64_t)));
    }
}

/**
 * @brief Sorts an array of uint64_t values in ascending order and returns the permutation applied.
 *
 * @param arr    First element of the array to sort in place.
 * @param tmp    First element of a temporary array of nitems elements.
 * @param idx    First element of the index array to be returned.
 * @param tdx    First element of a temporary index array of nitems elements.
 * @param nitems Number of elements in the array.
 */
static inline void order_uint64_t(uint64_t *VK_RESTRICT arr, uint64_t *VK_RESTRICT tmp, uint32_t *VK_RESTRICT idx, uint32_t *VK_RESTRICT tdx, uint32_t nitems)
{
    uint32_t cnt[RADIX_SORT_PASSES][256];
    uint64_t diff = radix_sort_count(arr, nitems, cnt);
    uint64_t *vsrc = arr;
    uint64_t *vdst = tmp;
    uint32_t *isrc = idx;
    uint32_t *idst = tdx;
    uint32_t i = 0;
    uint8_t b = 0;
    for (i = 0; i < nitems; i++)
    {
        idx[i] = i;
    }
    for (b = 0; b < RADIX_SORT_PASSES; b++)
    {
        if (((diff >> (b << 3)) & 0xff) == 0)
        {
            continue; // this byte is the same in every element
        }
        for (i = 0; i < nitems; i++)
        {
            uint64_t v = vsrc[i];
            uint32_t j = cnt[b][((v >> (b << 3)) & 0xff)]++;
            vdst[j] = v;
            idst[j] = isrc[i];
        }
        uint64_t *vswap = vsrc;
        vsrc = vdst;
        vdst = vswap;
        uint32_t *iswap = isrc;
        isrc = idst;
        idst = iswap;
    }
    if (vsrc != arr)
    {
        memcpy(arr, vsrc, (nitems * sizeof(uint64_t)));
        memcpy(idx, isrc, (nitems * sizeof(uint32_t)));
    }
}

/**
 * @brief Reverses an array of uint64_t values in place.
 *
 * @param arr    First element of the array to reverse.
 * @param nitems Number of elements in the array.
 */
static inline void reverse_uint64_t(uint64_t *arr, uint64_t nitems)
{
    uint64_t *last = (arr + nitems);
    while (arr != last)
    {
        --last;
        if (arr == last)
        {
            break;
        }
        uint64_t tmp = *last;
        *last = *arr;
        *arr++ = tmp;
    }
}

/**
 * @brief Removes all but the first element of every run of equal values.
 *
 * The elements are moved in place; the ones past the returned end are left
 * unspecified.
 *
 * @param arr    First element of the array to process.
 * @param nitems Number of elements in the array.
 *
 * @return Pointer past the last retained element.
 */
static inline uint64_t *unique_uint64_t(uint64_t *arr, uint64_t nitems)
{
    if (nitems == 0)
    {
        return arr;
    }
    const uint64_t *last = (arr + nitems);
    uint64_t *p = arr;
    while (++arr != last)
    {
        if (*p != *arr)
        {
            *(++p) = *arr;
        }
    }
    return ++p;
}

/**
 * @brief Writes the intersection of two sorted uint64_t arrays.
 *
 * @param a_arr    First element of the first array.
 * @param a_nitems Number of elements in the first array.
 * @param b_arr    First element of the second array.
 * @param b_nitems Number of elements in the second array.
 * @param o_arr    First element of the output array.
 *
 * @return Pointer past the last element written.
 */
static inline uint64_t *intersection_uint64_t(const uint64_t *VK_RESTRICT a_arr, uint64_t a_nitems, const uint64_t *VK_RESTRICT b_arr, uint64_t b_nitems, uint64_t *VK_RESTRICT o_arr)
{
    const uint64_t *a_last = (a_arr + a_nitems);
    const uint64_t *b_last = (b_arr + b_nitems);
    while ((a_arr != a_last) && (b_arr != b_last))
    {
        if (*a_arr < *b_arr)
        {
            ++a_arr;
            continue;
        }
        if (*a_arr == *b_arr)
        {
            *o_arr++ = *a_arr++;
        }
        ++b_arr;
    }
    return o_arr;
}

/**
 * @brief Writes the union of two sorted uint64_t arrays.
 *
 * @param a_arr    First element of the first array.
 * @param a_nitems Number of elements in the first array.
 * @param b_arr    First element of the second array.
 * @param b_nitems Number of elements in the second array.
 * @param o_arr    First element of the output array.
 *
 * @return Pointer past the last element written.
 */
static inline uint64_t *union_uint64_t(const uint64_t *VK_RESTRICT a_arr, uint64_t a_nitems, const uint64_t *VK_RESTRICT b_arr, uint64_t b_nitems, uint64_t *VK_RESTRICT o_arr)
{
    const uint64_t *a_last = (a_arr + a_nitems);
    const uint64_t *b_last = (b_arr + b_nitems);
    while ((a_arr != a_last) && (b_arr != b_last))
    {
        if (*a_arr < *b_arr)
        {
            *o_arr++ = *a_arr++;
            continue;
        }
        if (*a_arr > *b_arr)
        {
            *o_arr++ = *b_arr++;
            continue;
        }
        *o_arr++ = *a_arr++;
        b_arr++;
    }
    while (a_arr != a_last)
    {
        *o_arr++ = *a_arr++;
    }
    while (b_arr != b_last)
    {
        *o_arr++ = *b_arr++;
    }
    return o_arr;
}

#endif  // VARIANTKEY_SET_H
