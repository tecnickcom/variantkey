// VariantKey
//
// test_esid.c
//
// @category   Test
// @author     Nicola Asuni <info@tecnick.com>
// @link       https://github.com/tecnickcom/variantkey
// @license    MIT [LICENSE](https://raw.githubusercontent.com/tecnickcom/variantkey/main/LICENSE)

// Test for esid

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include "../src/variantkey/esid.h"

// returns current time in nanoseconds
uint64_t get_time()
{
    struct timespec t;
    (void) timespec_get(&t, TIME_UTC);
    return (((uint64_t)t.tv_sec * 1000000000) + (uint64_t)t.tv_nsec);
}

typedef struct esid_data_t
{
    size_t size;
    size_t start;
    size_t esize;
    uint64_t esid;
    uint64_t hsid;
    const char estr[11];
    const char str[37];
} __attribute__((packed)) __attribute__((aligned(128))) esid_data_t;

static const int k_esid_data_size = 36;

static const esid_data_t esid_data[] =
{
    {36,  0, 10, 0xa411493515597619, 0xb3a5fdb8808cb7dc, "0123456789", "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"},
    {36,  1, 10, 0xa4524d45565d8661, 0xb3a5fdb8808cb7dc, "123456789A", "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"},
    {36, 10, 10, 0xa8628e49669e8a6a, 0xd93e382010f46f32, "ABCDEFGHIJ", "0223456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"},
    {36, 25, 10, 0xac31cb3d35db7e39, 0xbfc379f4a71cb3be, "PQRSTUVWXY", "0133456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"},
    {36, 26, 10, 0xac72cf4d76df8e7a, 0xdba299d06b54215d, "QRSTUVWXYZ", "1123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"},
    {36, 35,  1, 0x1ec0000000000000, 0x996dcdc8ef7c89c1, "[",          "0123456789ABCDEFGHIJKLMNOPQRSTUVWXY["},
    { 6,  0,  6, 0x6411493515000000, 0xa81279395d9c891e, "012345",     "012345"                              },
    { 6,  1,  5, 0x54524d4540000000, 0xa81279395d9c891e, "12345",      "012345"                              },
    { 6,  3,  3, 0x34d4540000000000, 0xa81279395d9c891e, "345",        "012345"                              },
    { 6,  5,  1, 0x1540000000000000, 0xa81279395d9c891e, "5",          "012345"                              },
    {10,  0, 10, 0xafc1083105187209, 0x838c148028a06998, "_!\"#$%&'()"," !\"#$%&'()"                         },
    {10,  0, 10, 0xa28b30d38f411493, 0x96e42cf7aefeccf0, "*+,-./0123", "*+,-./0123"                          },
    {10,  0, 10, 0xa51559761969b71d, 0xb391c7f288e446ee, "456789:;<=", "456789:;<="                          },
    {10,  0, 10, 0xa79f8218a39259a7, 0xcc0de20381405e0a, ">?@ABCDEFG", ">?@ABCDEFG"                          },
    {10,  0, 10, 0xaa29aabb2dbafc31, 0xbbfc630eacb6c552, "HIJKLMNOPQ", "HIJKLMNOPQ"                          },
    {10,  0, 10, 0xacb3d35db7e39ebb, 0x803eda0a1781a117, "RSTUVWXYZ[", "RSTUVWXYZ["                          },
    {10,  0, 10, 0xaf3dfbf8218a3925, 0x878ef2947ee3cc2b, "\\]^_@ABCDE","\\]^_`abcde"                         },
    {10,  0, 10, 0xa9a7a29aabb2dbaf, 0xbaadaef07826969b, "FGHIJKLMNO", "fghijklmno"                          },
    {10,  0, 10, 0xac31cb3d35db7e39, 0xd3da4d5e28be3590, "PQRSTUVWXY", "pqrstuvwxy"                          },
    { 6,  0,  6, 0x6ebbf3dfbf000000, 0xeadc752a50c5b850, "Z[\\]^_",    "z{|}~\t"                             },
    {15,  0, 10, 0xa4524d45565d8452, 0x9af6ee553ba41827, "1234567812", "123456781234567"                     },
    {14,  0, 10, 0xa4524d45565d8452, 0xc94e10e9fe153fd2, "1234567812", "12345678123456"                      },
    {13,  0, 10, 0xa4524d45565d8452, 0x9b56411c7abdefe6, "1234567812", "1234567812345"                       },
    {12,  0, 10, 0xa4524d45565d8452, 0xcbec35944ff1c863, "1234567812", "123456781234"                        },
    {11,  0, 10, 0xa4524d45565d8452, 0x929920298cf56b9b, "1234567812", "12345678123"                         },
    {10,  0, 10, 0xa4524d45565d8452, 0xbdf006d50a33ad90, "1234567812", "1234567812"                          },
    { 9,  0,  9, 0x94524d45565d8440, 0xde094182e93557ae, "123456781",  "123456781"                           },
    { 8,  0,  8, 0x84524d45565d8000, 0xccbc926a73ece95c, "12345678",   "12345678"                            },
    { 7,  0,  7, 0x74524d45565c0000, 0xd18b960e2f99c279, "1234567",    "1234567"                             },
    { 6,  0,  6, 0x64524d4556000000, 0x811c9c02fcc22096, "123456",     "123456"                              },
    { 5,  0,  5, 0x54524d4540000000, 0xd863467dedd1cab1, "12345",      "12345"                               },
    { 4,  0,  4, 0x44524d4000000000, 0xbef404ecb71e4cd9, "1234",       "1234"                                },
    { 3,  0,  3, 0x34524c0000000000, 0xfad470644116fa54, "123",        "123"                                 },
    { 2,  0,  2, 0x2452000000000000, 0xd94ae1e7173e781d, "12",         "12"                                  },
    { 1,  0,  1, 0x1440000000000000, 0xfc73100baa96ad81, "1",          "1"                                   },
    { 0,  0,  0, 0x0000000000000000, 0x8000000000000000, "",           ""                                    },
};

typedef struct esid_num_data_t
{
    size_t size;
    size_t esize;
    uint64_t esid;
    const char estr[30];
    const char str[30];
} __attribute__((packed)) __attribute__((aligned(128))) esid_num_data_t;

static const int k_esid_num_data_size = 60;

static const esid_num_data_t esid_num_data[] =
{
    { 1,  1, 0x1680000000000000, ":",                     ":"},
    { 1,  1, 0x1840000000000000, "A",                     "A"},
    { 2,  2, 0x285a000000000000, "A:",                    "A:"},
    { 2,  2, 0x2691000000000000, ":1",                    ":1"},
    { 2,  2, 0x2862000000000000, "AB",                    "Ab"},
    { 3,  3, 0x3862680000000000, "AB:",                   "Ab:"},
    { 3,  3, 0x38628c0000000000, "ABC",                   "AbC"},
    { 4,  4, 0x48628da000000000, "ABC:",                  "AbC:"},
    { 4,  4, 0x48628e4000000000, "ABCD",                  "AbCd"},
    { 5,  5, 0x58628e4680000000, "ABCD:",                 "AbCd:"},
    { 5,  5, 0x58628e4940000000, "ABCDE",                 "AbCdE"},
    { 6,  6, 0x68628e495a000000, "ABCDE:",                "AbCdE:"},
    { 6,  6, 0x68628e4966000000, "ABCDEF",                "AbCdEf"},
    { 7,  7, 0x78628e4966680000, "ABCDEF:",               "AbCdEf:"},
    { 7,  7, 0x78628e49669c0000, "ABCDEFG",               "AbCdEfG"},
    { 8,  8, 0x88628e49669da000, "ABCDEFG:",              "AbCdEfG:"},
    { 8,  8, 0x88628e49669e8000, "ABCDEFGH",              "AbCdEfGh"},
    { 9,  9, 0x98628e49669e8680, "ABCDEFGH:",             "AbCdEfGh:"},
    { 5,  5, 0x58628da400000000, "ABC:0",                 "AbC:0"},
    { 5,  5, 0x58628da440000000, "ABC:1",                 "AbC:1"},
    {12, 12, 0xd8628c0000bc614e, "ABC:12345678",          "AbC:12345678"},
    {13, 13, 0xd8628c0008bc614e, "ABC:012345678",         "AbC:012345678"},
    {14, 14, 0xd8628c0010bc614e, "ABC:0012345678",        "AbC:0012345678"},
    {15, 15, 0xd8628c0018bc614e, "ABC:00012345678",       "AbC:00012345678"},
    {16, 16, 0xd8628c0020bc614e, "ABC:000012345678",      "AbC:000012345678"},
    {17, 17, 0xd8628c0028bc614e, "ABC:0000012345678",     "AbC:0000012345678"},
    {18, 18, 0xd8628c0030bc614e, "ABC:00000012345678",    "AbC:00000012345678"},
    {19, 19, 0xd8628c0038bc614e, "ABC:000000012345678",   "AbC:000000012345678"},
    {20, 19, 0xd8628c0038bc614e, "ABC:000000012345678",   "AbC:0000000012345678"},
    {21, 19, 0xd8628c0038bc614e, "ABC:000000012345678",   "AbC:00000000012345678"},
    {22, 19, 0xd8628c0038bc614e, "ABC:000000012345678",   "AbC:000000000012345678"},
    {23, 19, 0xd8628c0038bc614e, "ABC:000000012345678",   "AbC:0000000000012345678"},
    { 7,  7, 0x78628e495a400000, "ABCDE:0",               "AbCdE:0"},
    { 7,  7, 0x78628e495a440000, "ABCDE:1",               "AbCdE:1"},
    {14, 14, 0xf8628e4940bc614e, "ABCDE:12345678",        "AbCdE:12345678"},
    {15, 15, 0xf8628e4948bc614e, "ABCDE:012345678",       "AbCdE:012345678"},
    {16, 16, 0xf8628e4950bc614e, "ABCDE:0012345678",      "AbCdE:0012345678"},
    {17, 17, 0xf8628e4958bc614e, "ABCDE:00012345678",     "AbCdE:00012345678"},
    {18, 18, 0xf8628e4960bc614e, "ABCDE:000012345678",    "AbCdE:000012345678"},
    {19, 19, 0xf8628e4968bc614e, "ABCDE:0000012345678",   "AbCdE:0000012345678"},
    {20, 20, 0xf8628e4970bc614e, "ABCDE:00000012345678",  "AbCdE:00000012345678"},
    {21, 21, 0xf8628e4978bc614e, "ABCDE:000000012345678", "AbCdE:000000012345678"},
    {22, 21, 0xf8628e4978bc614e, "ABCDE:000000012345678", "AbCdE:0000000012345678"},
    {23, 21, 0xf8628e4978bc614e, "ABCDE:000000012345678", "AbCdE:00000000012345678"},
    {24, 21, 0xf8628e4978bc614e, "ABCDE:000000012345678", "AbCdE:000000000012345678"},
    {25, 21, 0xf8628e4978bc614e, "ABCDE:000000012345678", "AbCdE:0000000000012345678"},
    { 9,  9, 0x98628e49669da400, "ABCDEFG:0",             "AbCdEfG:0"},
    { 9,  9, 0x98628e49669da440, "ABCDEFG:1",             "AbCdEfG:1"},
    {16, 14, 0xf8628e4940bc614e, "ABCDE:12345678",        "AbCdEfG:12345678"},
    {17, 15, 0xf8628e4948bc614e, "ABCDE:012345678",       "AbCdEfG:012345678"},
    {18, 16, 0xf8628e4950bc614e, "ABCDE:0012345678",      "AbCdEfG:0012345678"},
    {19, 17, 0xf8628e4958bc614e, "ABCDE:00012345678",     "AbCdEfG:00012345678"},
    {20, 18, 0xf8628e4960bc614e, "ABCDE:000012345678",    "AbCdEfG:000012345678"},
    {21, 19, 0xf8628e4968bc614e, "ABCDE:0000012345678",   "AbCdEfG:0000012345678"},
    {22, 20, 0xf8628e4970bc614e, "ABCDE:00000012345678",  "AbCdEfG:00000012345678"},
    {23, 21, 0xf8628e4978bc614e, "ABCDE:000000012345678", "AbCdEfG:000000012345678"},
    {24, 21, 0xf8628e4978bc614e, "ABCDE:000000012345678", "AbCdEfG:0000000012345678"},
    {25, 21, 0xf8628e4978bc614e, "ABCDE:000000012345678", "AbCdEfG:00000000012345678"},
    {26, 21, 0xf8628e4978bc614e, "ABCDE:000000012345678", "AbCdEfG:000000000012345678"},
    {27, 21, 0xf8628e4978bc614e, "ABCDE:000000012345678", "AbCdEfG:0000000000012345678"},
};

int test_encode_string_id()
{
    int i = 0;
    int errors = 0;
    uint64_t esid = 0;
    for (i=0 ; i < k_esid_data_size; i++)
    {
        esid = encode_string_id(esid_data[i].str, esid_data[i].size, esid_data[i].start);
        if (esid != esid_data[i].esid)
        {
            (void) fprintf(stderr, "%s (%d): Expected 0x%016" PRIx64 ", got 0x%016" PRIx64 "\n", __func__, i, esid_data[i].esid, esid);
            ++errors;
        }
    }
    return errors;
}

void benchmark_encode_string_id()
{
    uint64_t esid = 0;
    uint64_t tstart = 0, tend = 0;
    int i = 0;
    int size = 1000;
    tstart = get_time();
    for (i=0 ; i < size; i++)
    {
        esid = encode_string_id("ABC0123456", 10, 0);
    }
    tend = get_time();
    (void) fprintf(stdout, " * %s : %lu ns/op (%" PRIx64 ")\n", __func__, (tend - tstart)/size, esid);
}

int test_encode_string_num_id()
{
    int i = 0;
    int errors = 0;
    uint64_t esid = 0;
    for (i=0 ; i < k_esid_num_data_size; i++)
    {
        esid = encode_string_num_id(esid_num_data[i].str, esid_num_data[i].size, ':');
        if (esid != esid_num_data[i].esid)
        {
            (void) fprintf(stderr, "%s (%d): Expected 0x%016" PRIx64 ", got 0x%016" PRIx64 "\n", __func__, i, esid_num_data[i].esid, esid);
            ++errors;
        }
    }
    return errors;
}

void benchmark_encode_string_num_id()
{
    uint64_t esid = 0;
    uint64_t tstart = 0, tend = 0;
    int i = 0;
    int size = 1000;
    tstart = get_time();
    for (i=0 ; i < size; i++)
    {
        esid = encode_string_num_id("AbCDE:000012345", 15, ':');
    }
    tend = get_time();
    (void) fprintf(stdout, " * %s : %lu ns/op (%" PRIx64 ")\n", __func__, (tend - tstart)/size, esid);
}

int test_decode_string_id()
{
    int i = 0;
    int errors = 0;
    char esid[11];
    size_t size = 0;
    for (i=0 ; i < k_esid_data_size; i++)
    {
        size = decode_string_id(esid_data[i].esid, esid);
        if (strcmp(esid, esid_data[i].estr) != 0)
        {
            (void) fprintf(stderr, "%s (%d): Expected %s, got %s\n", __func__, i, esid_data[i].estr, esid);
            ++errors;
        }
        if (size != esid_data[i].esize)
        {
            (void) fprintf(stderr, "%s (%d): Expected size %lu, got %lu\n", __func__, i, esid_data[i].esize, size);
            ++errors;
        }
    }
    return errors;
}

void benchmark_decode_string_id()
{
    char esid[11] = "";
    size_t len = 0;
    uint64_t tstart = 0, tend = 0;
    int i = 0;
    int size = 1000;
    tstart = get_time();
    for (i=0 ; i < size; i++)
    {
        len = decode_string_id(0x08628e49669e8a6a, esid);
    }
    tend = get_time();
    (void) fprintf(stdout, " * %s : %lu ns/op (%lu)\n", __func__, (tend - tstart)/size, len);
}

int test_decode_string_num_id()
{
    int i = 0;
    int errors = 0;
    char esid[30];
    size_t size = 0;
    for (i=0 ; i < k_esid_num_data_size; i++)
    {
        size = decode_string_id(esid_num_data[i].esid, esid);
        if (strcmp(esid, esid_num_data[i].estr) != 0)
        {
            (void) fprintf(stderr, "%s (%d): Expected %s, got %s\n", __func__, i, esid_num_data[i].estr, esid);
            ++errors;
        }
        if (size != esid_num_data[i].esize)
        {
            (void) fprintf(stderr, "%s (%d): Expected size %lu, got %lu\n", __func__, i, esid_num_data[i].esize, size);
            ++errors;
        }
    }
    return errors;
}

void benchmark_decode_string_num_id()
{
    char esid[30] = "";
    size_t len = 0;
    uint64_t tstart = 0, tend = 0;
    int i = 0;
    int size = 1000;
    tstart = get_time();
    for (i=0 ; i < size; i++)
    {
        len = decode_string_id(0xf8628e4978bc614e, esid);
    }
    tend = get_time();
    (void) fprintf(stdout, " * %s : %lu ns/op (%lu)\n", __func__, (tend - tstart)/size, len);
}

int test_hash_string_id()
{
    int i = 0;
    int errors = 0;
    uint64_t hsid = 0;
    for (i=0 ; i < k_esid_data_size; i++)
    {
        hsid = hash_string_id(esid_data[i].str, esid_data[i].size);
        if (hsid != esid_data[i].hsid)
        {
            (void) fprintf(stderr, "%s (%d): Expected 0x%016" PRIx64 ", got 0x%016" PRIx64 "\n", __func__, i, esid_data[i].hsid, hsid);
            ++errors;
        }
    }
    return errors;
}

void benchmark_hash_string_id()
{
    uint64_t hsid = 0;
    uint64_t tstart = 0, tend = 0;
    int i = 0;
    int size = 1000;
    tstart = get_time();
    for (i=0 ; i < size; i++)
    {
        hsid = hash_string_id("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ", 36);
    }
    tend = get_time();
    (void) fprintf(stdout, " * %s : %lu ns/op (%" PRIx64 ")\n", __func__, (tend - tstart)/size, hsid);
}

int main()
{
    int errors = 0;

    errors += test_encode_string_id();
    errors += test_decode_string_id();
    errors += test_encode_string_num_id();
    errors += test_decode_string_num_id();
    errors += test_hash_string_id();

    benchmark_encode_string_id();
    benchmark_encode_string_num_id();
    benchmark_decode_string_id();
    benchmark_decode_string_num_id();
    benchmark_hash_string_id();

    return errors;
}
