// Benchmark tool for VariantKey vs CSV lookups
//
// vk_csv_lookup_bench.c
//
// @category   Tools
// @author     Nicola Asuni <info@tecnick.com>
// @license    MIT (see LICENSE)
// @link       https://github.com/tecnickcom/variantkey

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../../src/variantkey/binsearch.h"
#include "../../src/variantkey/variantkey.h"

#define DEFAULT_BENCH_ROWS 100000UL
#define DEFAULT_BENCH_QUERIES 200UL
#define BENCH_ROUNDS 3

#define COL_FILE "vk_lookup_test.bin"
#define CSV_FILE "vk_lookup_test.csv"

typedef struct row_t
{
    char chrom[3];
    uint8_t chrom_len;
    uint32_t pos;
    char ref;
    char alt;
    uint32_t payload;
    uint64_t vk;
} row_t;

typedef struct csv_query_t
{
    char chrom[3];
    uint8_t chrom_len;
    uint32_t pos;
    char ref;
    char alt;
} csv_query_t;

static uint64_t get_time_ns(void)
{
    struct timespec t;
    (void)timespec_get(&t, TIME_UTC);
    return (((uint64_t)t.tv_sec * 1000000000ULL) + (uint64_t)t.tv_nsec);
}

static int cmp_row_by_vk(const void *a, const void *b)
{
    const row_t *ra = (const row_t *)a;
    const row_t *rb = (const row_t *)b;
    if (ra->vk < rb->vk)
    {
        return -1;
    }
    if (ra->vk > rb->vk)
    {
        return 1;
    }
    return 0;
}

static uint32_t lcg_next(uint32_t x)
{
    return (x * 1664525U) + 1013904223U;
}

static void fill_chrom_string(uint8_t chrom_code, char *chrom, uint8_t *len)
{
    if (chrom_code < 10)
    {
        chrom[0] = (char)('0' + chrom_code);
        chrom[1] = '\0';
        *len = 1;
        return;
    }
    chrom[0] = (char)('0' + (chrom_code / 10));
    chrom[1] = (char)('0' + (chrom_code % 10));
    chrom[2] = '\0';
    *len = 2;
}

static int build_dataset(row_t *rows, uint64_t nrows)
{
    static const char bases[] = {'A', 'C', 'G', 'T'};
    uint64_t i = 0;

    for (i = 0; i < nrows; ++i)
    {
        uint8_t chrom_code = (uint8_t)((i % 22U) + 1U);
        uint32_t pos = (uint32_t)(1000U + i * 2U + (i % 17U));
        char ref = bases[i & 3U];
        char alt = bases[(i + 1U) & 3U];

        fill_chrom_string(chrom_code, rows[i].chrom, &rows[i].chrom_len);
        rows[i].pos = pos;
        rows[i].ref = ref;
        rows[i].alt = alt;
        rows[i].payload = (uint32_t)(i + 1U);
        rows[i].vk = variantkey(rows[i].chrom, rows[i].chrom_len, rows[i].pos, &rows[i].ref, 1U, &rows[i].alt, 1U);
    }

    qsort(rows, (size_t)nrows, sizeof(row_t), cmp_row_by_vk);

    for (i = 0; i < nrows; ++i)
    {
        rows[i].payload = (uint32_t)(i + 1U);
    }

    return 0;
}

static int write_columnar_file(const char *filename, const row_t *rows, uint64_t nrows)
{
    FILE *f = fopen(filename, "wb");
    if (f == NULL)
    {
        (void)fprintf(stderr, "unable to open %s for writing\n", filename);
        return 1;
    }

    uint64_t i = 0;
    for (i = 0; i < nrows; ++i)
    {
        if (fwrite(&rows[i].vk, sizeof(uint64_t), 1U, f) != 1U)
        {
            (void)fprintf(stderr, "error writing vk column\n");
            (void)fclose(f);
            return 1;
        }
    }

    for (i = 0; i < nrows; ++i)
    {
        if (fwrite(&rows[i].payload, sizeof(uint32_t), 1U, f) != 1U)
        {
            (void)fprintf(stderr, "error writing payload column\n");
            (void)fclose(f);
            return 1;
        }
    }

    (void)fclose(f);
    return 0;
}

static int write_csv_file(const char *filename, const row_t *rows, uint64_t nrows)
{
    FILE *f = fopen(filename, "w");
    if (f == NULL)
    {
        (void)fprintf(stderr, "unable to open %s for writing\n", filename);
        return 1;
    }

    uint64_t i = 0;
    for (i = 0; i < nrows; ++i)
    {
        if (fprintf(f, "%s,%" PRIu32 ",%c,%c,%" PRIu32 "\n", rows[i].chrom, rows[i].pos, rows[i].ref, rows[i].alt, rows[i].payload) < 0)
        {
            (void)fprintf(stderr, "error writing csv row\n");
            (void)fclose(f);
            return 1;
        }
    }

    (void)fclose(f);
    return 0;
}

static int load_text_file(const char *filename, char **data, size_t *size)
{
    FILE *f = fopen(filename, "rb");
    if (f == NULL)
    {
        return 1;
    }

    if (fseek(f, 0, SEEK_END) != 0)
    {
        (void)fclose(f);
        return 1;
    }

    long fsize = ftell(f);
    if (fsize < 0)
    {
        (void)fclose(f);
        return 1;
    }
    if (fseek(f, 0, SEEK_SET) != 0)
    {
        (void)fclose(f);
        return 1;
    }

    *size = (size_t)fsize;
    *data = (char *)malloc(*size + 1U);
    if (*data == NULL)
    {
        (void)fclose(f);
        return 1;
    }

    if (fread(*data, 1U, *size, f) != *size)
    {
        free(*data);
        *data = NULL;
        (void)fclose(f);
        return 1;
    }
    (*data)[*size] = '\0';

    (void)fclose(f);
    return 0;
}

static uint32_t parse_uint32_token(const char **p, const char *end)
{
    uint32_t v = 0;
    while ((*p < end) && (**p >= '0') && (**p <= '9'))
    {
        v = (v * 10U) + (uint32_t)(**p - '0');
        ++(*p);
    }
    return v;
}

static uint32_t lookup_csv_payload(
    const char *csv,
    size_t csv_size,
    const csv_query_t *q)
{
    const char *p = csv;
    const char *end = csv + csv_size;

    while (p < end)
    {
        const char *chrom_start = p;
        while ((p < end) && (*p != ','))
        {
            ++p;
        }
        if (p >= end)
        {
            break;
        }

        size_t chrom_len = (size_t)(p - chrom_start);
        ++p;

        uint32_t pos = parse_uint32_token(&p, end);
        if ((p >= end) || (*p != ','))
        {
            break;
        }
        ++p;

        if (p >= end)
        {
            break;
        }
        char ref = *p;
        ++p;
        if ((p >= end) || (*p != ','))
        {
            break;
        }
        ++p;

        if (p >= end)
        {
            break;
        }
        char alt = *p;
        ++p;
        if ((p >= end) || (*p != ','))
        {
            break;
        }
        ++p;

        uint32_t payload = parse_uint32_token(&p, end);
        while ((p < end) && (*p != '\n'))
        {
            ++p;
        }
        if ((p < end) && (*p == '\n'))
        {
            ++p;
        }

        if ((chrom_len == q->chrom_len)
                && (memcmp(chrom_start, q->chrom, chrom_len) == 0)
                && (pos == q->pos)
                && (ref == q->ref)
                && (alt == q->alt))
        {
            return payload;
        }
    }

    return 0;
}

static uint32_t lookup_columnar_payload(
    const uint64_t *vk_col,
    const uint32_t *payload_col,
    uint64_t nrows,
    const csv_query_t *q)
{
    uint64_t first = 0;
    uint64_t last = nrows;
    uint64_t key = variantkey(q->chrom, q->chrom_len, q->pos, &q->ref, 1U, &q->alt, 1U);

    uint64_t found = col_find_first_uint64_t(vk_col, &first, &last, key);
    if ((found >= nrows) || (vk_col[found] != key))
    {
        return 0;
    }

    return payload_col[found];
}

static void build_queries(const row_t *rows, uint64_t nrows, csv_query_t *queries, uint64_t nqueries)
{
    uint64_t i = 0;
    uint32_t seed = 0x12345678U;
    for (i = 0; i < nqueries; ++i)
    {
        seed = lcg_next(seed);
        uint64_t idx = (uint64_t)(seed % (uint32_t)nrows);
        queries[i].chrom[0] = rows[idx].chrom[0];
        queries[i].chrom[1] = rows[idx].chrom[1];
        queries[i].chrom[2] = rows[idx].chrom[2];
        queries[i].chrom_len = rows[idx].chrom_len;
        queries[i].pos = rows[idx].pos;
        queries[i].ref = rows[idx].ref;
        queries[i].alt = rows[idx].alt;
    }
}

static uint64_t benchmark_offset(const csv_query_t *queries, uint64_t nqueries)
{
    volatile uint64_t sum = 0;
    uint64_t t0 = get_time_ns();
    uint64_t i = 0;
    for (i = 0; i < nqueries; ++i)
    {
        sum += (uint64_t)queries[i].pos;
    }
    uint64_t t1 = get_time_ns();
    (void)sum;
    return t1 - t0;
}

int main(int argc, char **argv)
{
    int ret = 0;
    row_t *rows = NULL;
    csv_query_t *queries = NULL;
    uint64_t nrows = DEFAULT_BENCH_ROWS;
    uint64_t nqueries = DEFAULT_BENCH_QUERIES;

    if (argc > 1)
    {
        nrows = strtoull(argv[1], NULL, 10);
    }
    if (argc > 2)
    {
        nqueries = strtoull(argv[2], NULL, 10);
    }
    if ((nrows == 0U) || (nqueries == 0U))
    {
        (void)fprintf(stderr, "usage: %s [rows] [queries]\n", argv[0]);
        return 1;
    }

    rows = (row_t *)malloc((size_t)nrows * sizeof(row_t));
    queries = (csv_query_t *)malloc((size_t)nqueries * sizeof(csv_query_t));
    if ((rows == NULL) || (queries == NULL))
    {
        (void)fprintf(stderr, "allocation error\n");
        free(rows);
        free(queries);
        return 1;
    }

    if (build_dataset(rows, nrows) != 0)
    {
        free(rows);
        free(queries);
        return 1;
    }

    if (write_columnar_file(COL_FILE, rows, nrows) != 0)
    {
        free(rows);
        free(queries);
        return 1;
    }

    if (write_csv_file(CSV_FILE, rows, nrows) != 0)
    {
        free(rows);
        free(queries);
        return 1;
    }

    build_queries(rows, nrows, queries, nqueries);

    mmfile_t mf = {0};
    mf.ncols = 2;
    mf.ctbytes[0] = 8;
    mf.ctbytes[1] = 4;
    mmap_binfile(COL_FILE, &mf);
    if ((mf.src == MAP_FAILED) || (mf.nrows != nrows))
    {
        (void)fprintf(stderr, "columnar mmap error\n");
        free(rows);
        free(queries);
        return 1;
    }

    const uint64_t *vk_col = (const uint64_t *)(mf.src + mf.index[0]);
    const uint32_t *payload_col = (const uint32_t *)(mf.src + mf.index[1]);

    char *csv_data = NULL;
    size_t csv_size = 0;
    if (load_text_file(CSV_FILE, &csv_data, &csv_size) != 0)
    {
        (void)fprintf(stderr, "csv load error\n");
        (void)munmap_binfile(mf);
        free(rows);
        free(queries);
        return 1;
    }

    uint64_t offset = benchmark_offset(queries, nqueries);
    (void)fprintf(stdout, "rows=%" PRIu64 ", queries=%" PRIu64 ", baseline=%" PRIu64 " ns\n", nrows, nqueries, offset);

    int r = 0;
    for (r = 0; r < BENCH_ROUNDS; ++r)
    {
        uint64_t i = 0;
        volatile uint64_t sum = 0;
        uint64_t t0 = get_time_ns();
        for (i = 0; i < nqueries; ++i)
        {
            sum += (uint64_t)lookup_columnar_payload(vk_col, payload_col, mf.nrows, &queries[i]);
        }
        uint64_t t1 = get_time_ns();
        uint64_t dt = (t1 - t0 > offset) ? (t1 - t0 - offset) : (t1 - t0);
        (void)fprintf(stdout, "variantkey+columnar round=%d sum=%" PRIu64 " time=%" PRIu64 " ns (%" PRIu64 " ns/op)\n", r + 1, sum, dt, dt / nqueries);
    }

    uint64_t last_csv_ns = 0;
    for (r = 0; r < BENCH_ROUNDS; ++r)
    {
        uint64_t i = 0;
        volatile uint64_t sum = 0;
        uint64_t t0 = get_time_ns();
        for (i = 0; i < nqueries; ++i)
        {
            sum += (uint64_t)lookup_csv_payload(csv_data, csv_size, &queries[i]);
        }
        uint64_t t1 = get_time_ns();
        uint64_t dt = (t1 - t0 > offset) ? (t1 - t0 - offset) : (t1 - t0);
        last_csv_ns = dt;
        (void)fprintf(stdout, "csv(chrom,pos,ref,alt) round=%d sum=%" PRIu64 " time=%" PRIu64 " ns (%" PRIu64 " ns/op)\n", r + 1, sum, dt, dt / nqueries);
    }

    uint64_t i = 0;
    volatile uint64_t sum_vk = 0;
    volatile uint64_t sum_csv = 0;
    uint64_t t0 = get_time_ns();
    for (i = 0; i < nqueries; ++i)
    {
        sum_vk += (uint64_t)lookup_columnar_payload(vk_col, payload_col, mf.nrows, &queries[i]);
    }
    uint64_t t1 = get_time_ns();
    uint64_t vk_ns = (t1 - t0 > offset) ? (t1 - t0 - offset) : (t1 - t0);

    t0 = get_time_ns();
    for (i = 0; i < nqueries; ++i)
    {
        sum_csv += (uint64_t)lookup_csv_payload(csv_data, csv_size, &queries[i]);
    }
    t1 = get_time_ns();
    last_csv_ns = (t1 - t0 > offset) ? (t1 - t0 - offset) : (t1 - t0);

    if (sum_vk != sum_csv)
    {
        (void)fprintf(stderr, "validation failed: variantkey_sum=%" PRIu64 " csv_sum=%" PRIu64 "\n", sum_vk, sum_csv);
        ret = 1;
    }
    else
    {
        double speedup = (vk_ns == 0U) ? 0.0 : ((double)last_csv_ns / (double)vk_ns);
        (void)fprintf(stdout, "speedup(csv / variantkey) = %.2fx\n", speedup);
    }

    free(csv_data);
    (void)munmap_binfile(mf);
    free(rows);
    free(queries);

    return ret;
}
