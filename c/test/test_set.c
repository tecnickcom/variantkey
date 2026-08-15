// VariantKey
//
// test_set.c
//
// @category   Test
// @author     Nicola Asuni <info@tecnick.com>
// @link       https://github.com/tecnickcom/variantkey
// @license    MIT [LICENSE](https://raw.githubusercontent.com/tecnickcom/variantkey/main/LICENSE)

// Test for set

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include "../src/variantkey/set.h"

// returns current time in nanoseconds
uint64_t get_time()
{
    struct timespec t;
    (void) timespec_get(&t, TIME_UTC);
    return (((uint64_t)t.tv_sec * 1000000000) + (uint64_t)t.tv_nsec);
}

int test_sort_uint64_t()
{
    int errors = 0;
    uint64_t arr[10] = {8,1,9,3,2,7,4,0,5,6};
    uint64_t tmp[10];
    sort_uint64_t(arr, tmp, 10);
    uint32_t i = 0;
    for(i = 0; i < 10; i++)
    {
        if (arr[i] != i)
        {
            (void) fprintf(stderr, "%s : Expected %" PRIu32 ", got %" PRIu64 "\n", __func__, i, arr[i]);
            ++errors;
        }
    }
    return errors;
}

enum { BENCH_NITEMS = 100000 };

void benchmark_sort_uint64_t()
{
    const uint32_t nitems = BENCH_NITEMS;
    // Static, not automatic: these arrays are 800 KB each and would overflow the
    // default stack limit on some platforms.
    static uint64_t arr[BENCH_NITEMS], tmp[BENCH_NITEMS];
    uint32_t i = 0;
    for (i = 0; i < nitems; i++)
    {
        arr[i] = (uint64_t)(nitems - i);
    }
    uint64_t tstart = 0, tend = 0;
    tstart = get_time();
    sort_uint64_t(arr, tmp, nitems);
    tend = get_time();
    (void) fprintf(stdout, " * %s : %" PRIu64 " ns/op\n", __func__, (tend - tstart)/nitems);
}

int test_order_uint64_t()
{
    int errors = 0;
    uint64_t tmp[10], arr[10] =
    {
        0xfffffffffffffff0, // 0
        0xffffffff00000000, // 1
        0xffffffffffffffff, // 2
        0xffffffffff000000, // 3
        0xfffffffff0000000, // 4
        0xffffffffffffff00, // 5
        0xfffffffffff00000, // 6
        0x0000000000000000, // 7
        0xffffffffffff0000, // 8
        0xfffffffffffff000, // 9
    };
    uint32_t tdx[10], idx[10];
    uint64_t exp[10] =
    {
        0x0000000000000000, // 7
        0xffffffff00000000, // 1
        0xfffffffff0000000, // 4
        0xffffffffff000000, // 3
        0xfffffffffff00000, // 6
        0xffffffffffff0000, // 8
        0xfffffffffffff000, // 9
        0xffffffffffffff00, // 5
        0xfffffffffffffff0, // 0
        0xffffffffffffffff, // 2
    };
    order_uint64_t(arr, tmp, idx, tdx, 10);
    uint32_t i = 0;
    for(i = 0; i < 10; i++)
    {
        if (arr[i] != exp[i])
        {
            (void) fprintf(stderr, "%s (%" PRIu32 "): Expected value %" PRIu64 ", got %" PRIu64 "\n", __func__, i, exp[i], arr[i]);
            ++errors;
        }
    }
    uint32_t edx[10] = {7,1,4,3,6,8,9,5,0,2};
    for(i = 0; i < 10; i++)
    {
        if (idx[i] != edx[i])
        {
            (void) fprintf(stderr, "%s (%" PRIu32 "): Expected index %" PRIu32 ", got %" PRIu32 "\n", __func__, i, edx[i], idx[i]);
            ++errors;
        }
    }
    return errors;
}

int test_reverse_uint64_t()
{
    int errors = 0;
    uint64_t arr[9] = {0,1,2,3,4,5,6,7,8};
    reverse_uint64_t(arr, 9);
    uint64_t i = 0;
    for(i = 0; i < 9; i++)
    {
        if (arr[i] != (8 - i))
        {
            (void) fprintf(stderr, "%s : Expected %" PRIu64 ", got %" PRIu64 "\n", __func__, (8 - i), arr[i]);
            ++errors;
        }
    }
    return errors;
}

int test_unique_uint64_t()
{
    int errors = 0;
    uint64_t arr[22] = {0,1,2,2,3,3,3,4,4,4,4,5,5,5,5,5,5,6,7,8,9,9};
    uint64_t *p = unique_uint64_t(arr, 22);
    uint64_t n = (p - arr);
    if (n != 10)
    {
        (void) fprintf(stderr, "%s : Expected 10, got %" PRIu64 "\n", __func__, n);
        ++errors;
    }
    uint64_t i = 0;
    for(i = 0; i < n; i++)
    {
        if (arr[i] != i)
        {
            (void) fprintf(stderr, "%s : Expected %" PRIu64 ", got %" PRIu64 "\n", __func__, i, arr[i]);
            ++errors;
        }
    }
    return errors;
}

int test_unique_uint64_t_zero()
{
    int errors = 0;
    uint64_t arr[1] = {0};
    uint64_t *p = unique_uint64_t(arr, 0);
    uint64_t n = (p - arr);
    if (n != 0)
    {
        (void) fprintf(stderr, "%s : Expected 0, got %" PRIu64 "\n", __func__, n);
        ++errors;
    }
    return errors;
}

int test_intersection_uint64_t()
{
    int errors = 0;
    uint64_t a_arr[11] = {0,1,2,3,3,4,5,6,7,8,9};
    uint64_t b_arr[7] = {0,3,3,5,6,6,9};
    uint64_t o_arr[6] = {0};
    uint64_t *p = intersection_uint64_t(a_arr, 11, b_arr, 7, o_arr);
    uint64_t n = (p - o_arr);
    if (n != 6)
    {
        (void) fprintf(stderr, "%s : Expected 5, got %" PRIu64 "\n", __func__, n);
        ++errors;
    }
    uint64_t i = 0;
    uint64_t e[6] = {0,3,3,5,6,9};
    for(i = 0; i < n; i++)
    {
        if (o_arr[i] != e[i])
        {
            (void) fprintf(stderr, "%s (%" PRIu64 "): Expected %" PRIu64 ", got %" PRIu64 "\n", __func__, i, e[i], o_arr[i]);
            ++errors;
        }
    }
    return errors;
}

int test_union_uint64_t()
{
    int errors = 0;
    uint64_t a_arr[7] = {0,2,3,3,5,8,9};
    uint64_t b_arr[7] = {0,1,4,5,6,6,7};
    uint64_t o_arr[11] = {0};
    uint64_t *p = union_uint64_t(a_arr, 6, b_arr, 7, o_arr);
    uint64_t n = (p - o_arr);
    if (n != 11)
    {
        (void) fprintf(stderr, "%s : Expected 10, got %" PRIu64 "\n", __func__, n);
        ++errors;
    }
    uint64_t i = 0;
    uint64_t e[11] = {0,1,2,3,3,4,5,6,6,7,8};
    for(i = 0; i < n; i++)
    {
        if (o_arr[i] != e[i])
        {
            (void) fprintf(stderr, "%s (%" PRIu64 "): Expected %" PRIu64 ", got %" PRIu64 "\n", __func__, i, e[i], o_arr[i]);
            ++errors;
        }
    }
    return errors;
}

int test_union_uint64_t_ba()
{
    int errors = 0;
    uint64_t a_arr[6] = {0,1,4,5,6,7};
    uint64_t b_arr[7] = {0,2,3,5,8,9,9};
    uint64_t o_arr[20] = {0};
    uint64_t *p = union_uint64_t(a_arr, 6, b_arr, 7, o_arr);
    uint64_t n = (p - o_arr);
    if (n != 11)
    {
        (void) fprintf(stderr, "%s : Expected 5, got %" PRIu64 "\n", __func__, n);
        ++errors;
    }
    uint64_t i = 0;
    uint64_t e[11] = {0,1,2,3,4,5,6,7,8,9,9};
    for(i = 0; i < n; i++)
    {
        if (o_arr[i] != e[i])
        {
            (void) fprintf(stderr, "%s (%" PRIu64 "): Expected %" PRIu64 ", got %" PRIu64 "\n", __func__, i, e[i], o_arr[i]);
            ++errors;
        }
    }
    return errors;
}

// An empty array is left untouched.
int test_reverse_uint64_t_zero()
{
    int errors = 0;
    uint64_t arr[1] = {7};
    reverse_uint64_t(arr, 0);
    if (arr[0] != 7)
    {
        (void) fprintf(stderr, "%s : Expected 7, got %" PRIu64 "\n", __func__, arr[0]);
        ++errors;
    }
    return errors;
}

// The second array is exhausted before the first one.
int test_intersection_uint64_t_shorter_second()
{
    int errors = 0;
    uint64_t a_arr[4] = {1,2,3,4};
    uint64_t b_arr[2] = {1,2};
    uint64_t o_arr[2] = {0};
    uint64_t *p = intersection_uint64_t(a_arr, 4, b_arr, 2, o_arr);
    uint64_t n = (p - o_arr);
    if (n != 2)
    {
        (void) fprintf(stderr, "%s : Expected 2, got %" PRIu64 "\n", __func__, n);
        ++errors;
    }
    if ((o_arr[0] != 1) || (o_arr[1] != 2))
    {
        (void) fprintf(stderr, "%s : Expected {1,2}, got {%" PRIu64 ",%" PRIu64 "}\n", __func__, o_arr[0], o_arr[1]);
        ++errors;
    }
    return errors;
}


// Values that differ in every byte, so no radix pass can be skipped and the
// result lands back in the caller's array (even number of passes).
int test_sort_uint64_t_wide()
{
    int errors = 0;
    uint64_t tmp[10], arr[10] =
    {
        0xfffffffffffffff0, 0xffffffff00000000, 0xffffffffffffffff, 0xffffffffff000000,
        0xfffffffff0000000, 0xffffffffffffff00, 0xfffffffffff00000, 0x0000000000000000,
        0xffffffffffff0000, 0xfffffffffffff000,
    };
    uint64_t exp[10] =
    {
        0x0000000000000000, 0xffffffff00000000, 0xfffffffff0000000, 0xffffffffff000000,
        0xfffffffffff00000, 0xffffffffffff0000, 0xfffffffffffff000, 0xffffffffffffff00,
        0xfffffffffffffff0, 0xffffffffffffffff,
    };
    sort_uint64_t(arr, tmp, 10);
    uint32_t i = 0;
    for (i = 0; i < 10; i++)
    {
        if (arr[i] != exp[i])
        {
            (void) fprintf(stderr, "%s (%" PRIu32 "): Expected %" PRIu64 ", got %" PRIu64 "\n", __func__, i, exp[i], arr[i]);
            ++errors;
        }
    }
    return errors;
}

// An empty array is left untouched and no pass runs at all.
int test_sort_uint64_t_zero()
{
    int errors = 0;
    uint64_t arr[1] = {7};
    uint64_t tmp[1] = {0};
    sort_uint64_t(arr, tmp, 0);
    if (arr[0] != 7)
    {
        (void) fprintf(stderr, "%s : Expected 7, got %" PRIu64 "\n", __func__, arr[0]);
        ++errors;
    }
    return errors;
}

// Values that only differ in the lowest byte, so seven of the eight passes are
// skipped and the result ends up in the temporary buffer (odd number of passes).
int test_order_uint64_t_narrow()
{
    int errors = 0;
    uint64_t tmp[6], arr[6] = {5, 3, 1, 4, 2, 0};
    uint32_t tdx[6], idx[6];
    uint32_t edx[6] = {5, 2, 4, 1, 3, 0};
    order_uint64_t(arr, tmp, idx, tdx, 6);
    uint32_t i = 0;
    for (i = 0; i < 6; i++)
    {
        if (arr[i] != i)
        {
            (void) fprintf(stderr, "%s (%" PRIu32 "): Expected value %" PRIu32 ", got %" PRIu64 "\n", __func__, i, i, arr[i]);
            ++errors;
        }
        if (idx[i] != edx[i])
        {
            (void) fprintf(stderr, "%s (%" PRIu32 "): Expected index %" PRIu32 ", got %" PRIu32 "\n", __func__, i, edx[i], idx[i]);
            ++errors;
        }
    }
    return errors;
}

// Duplicate keys must keep their original relative order (stable sort).
int test_order_uint64_t_duplicates()
{
    int errors = 0;
    uint64_t tmp[5], arr[5] = {2, 1, 2, 1, 2};
    uint32_t tdx[5], idx[5];
    uint64_t eval[5] = {1, 1, 2, 2, 2};
    uint32_t edx[5] = {1, 3, 0, 2, 4};
    order_uint64_t(arr, tmp, idx, tdx, 5);
    uint32_t i = 0;
    for (i = 0; i < 5; i++)
    {
        if (arr[i] != eval[i])
        {
            (void) fprintf(stderr, "%s (%" PRIu32 "): Expected value %" PRIu64 ", got %" PRIu64 "\n", __func__, i, eval[i], arr[i]);
            ++errors;
        }
        if (idx[i] != edx[i])
        {
            (void) fprintf(stderr, "%s (%" PRIu32 "): Expected index %" PRIu32 ", got %" PRIu32 "\n", __func__, i, edx[i], idx[i]);
            ++errors;
        }
    }
    return errors;
}

// Two arrays with no element in common.
int test_intersection_uint64_t_disjoint()
{
    int errors = 0;
    uint64_t a_arr[3] = {1, 3, 5};
    uint64_t b_arr[3] = {2, 4, 6};
    uint64_t o_arr[3] = {0};
    uint64_t *p = intersection_uint64_t(a_arr, 3, b_arr, 3, o_arr);
    if ((p - o_arr) != 0)
    {
        (void) fprintf(stderr, "%s : Expected 0, got %" PRIu64 "\n", __func__, (uint64_t)(p - o_arr));
        ++errors;
    }
    return errors;
}

// An empty input array on either side.
int test_set_uint64_t_empty()
{
    int errors = 0;
    uint64_t a_arr[3] = {1, 2, 3};
    uint64_t o_arr[3] = {0};
    uint64_t *p = intersection_uint64_t(a_arr, 3, a_arr, 0, o_arr);
    if ((p - o_arr) != 0)
    {
        (void) fprintf(stderr, "%s : Expected empty intersection, got %" PRIu64 "\n", __func__, (uint64_t)(p - o_arr));
        ++errors;
    }
    p = union_uint64_t(a_arr, 3, a_arr, 0, o_arr);
    if ((p - o_arr) != 3)
    {
        (void) fprintf(stderr, "%s : Expected 3, got %" PRIu64 "\n", __func__, (uint64_t)(p - o_arr));
        ++errors;
    }
    return errors;
}

int main()
{
    int errors = 0;

    errors += test_sort_uint64_t();
    errors += test_sort_uint64_t_wide();
    errors += test_sort_uint64_t_zero();
    errors += test_order_uint64_t_narrow();
    errors += test_order_uint64_t_duplicates();
    errors += test_intersection_uint64_t_disjoint();
    errors += test_set_uint64_t_empty();
    errors += test_order_uint64_t();
    errors += test_reverse_uint64_t();
    errors += test_reverse_uint64_t_zero();
    errors += test_unique_uint64_t();
    errors += test_unique_uint64_t_zero();
    errors += test_intersection_uint64_t();
    errors += test_intersection_uint64_t_shorter_second();
    errors += test_union_uint64_t();
    errors += test_union_uint64_t_ba();

    benchmark_sort_uint64_t();

    return errors;
}
