// VariantKey
//
// test_vkbin_unit.c
//
// @category   Test
// @author     Nicola Asuni <info@tecnick.com>
// @link       https://github.com/tecnickcom/variantkey
// @license    MIT [LICENSE](https://raw.githubusercontent.com/tecnickcom/variantkey/main/LICENSE)

/*
 * Unit test of the vkbin command line tool.
 *
 * The tool source is included here with its entry point renamed, so that the
 * internal functions can be called directly and the library calls it makes can
 * be failed on demand. The error paths are only reachable this way: a write
 * failure cannot be provoked from the command line.
 *
 * The cases below expect the tool to report a failure, so its diagnostics are
 * printed on the standard error while the test runs.
 */

// Matches the definition in the tool, so that both are compiled against the
// same file offset type.
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64 // NOLINT(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
#endif

#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../src/variantkey/binsearch.h"
#include "../src/variantkey/variantkey.h"

//!< Input file written by the test cases.
#define TEST_TSV "test_vkbin_unit.tsv"

//!< Output file built by the test cases.
#define TEST_BIN "test_vkbin_unit.bin"

//!< Scratch file used by the stream test cases.
#define TEST_TMP "test_vkbin_unit.tmp"

//!< Size of the argument buffer of run_main.
enum { TEST_MAX_ARGS_LEN = 8192 };

//!< Capacity of the argument vector of run_main.
enum { TEST_MAX_ARGS = 320 };

/*
 * Library call interception.
 *
 * Each intercepted function counts its calls and fails the one whose number
 * matches the armed value, which is zero while the interception is off. The
 * counts are those of a single run, because run_main and the helpers below
 * clear the configuration once the code under test has returned.
 */

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
static int cnt_malloc = 0;
static int cnt_fopen = 0;
static int cnt_fclose = 0;
static int cnt_fwrite = 0;
static int cnt_fputc = 0;
static int cnt_fseek = 0;
static int cnt_fflush = 0;
static int cnt_ferror = 0;
static int cnt_rename = 0;
static int cnt_snprintf = 0;

static int fail_malloc = 0;
static int fail_fopen = 0;
static int fail_fclose = 0;
static int fail_fwrite = 0;
static int fail_fputc = 0;
static int fail_fseek = 0;
static int fail_fflush = 0;
static int fail_ferror = 0;
static int fail_rename = 0;
static int fail_snprintf = 0;

//!< Value assigned to errno by a failing call.
static int fail_errno = 0;
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

/**
 * Count an intercepted call and tell whether it must fail.
 */
static bool tst_fail(int *cnt, int at)
{
    ++(*cnt);
    if ((at == 0) || (*cnt != at))
    {
        return false;
    }
    errno = fail_errno;
    return true;
}

/**
 * Clear the interception counters and the armed failures.
 */
static void reset_faults(void)
{
    cnt_malloc = 0;
    cnt_fopen = 0;
    cnt_fclose = 0;
    cnt_fwrite = 0;
    cnt_fputc = 0;
    cnt_fseek = 0;
    cnt_fflush = 0;
    cnt_ferror = 0;
    cnt_rename = 0;
    cnt_snprintf = 0;
    fail_malloc = 0;
    fail_fopen = 0;
    fail_fclose = 0;
    fail_fwrite = 0;
    fail_fputc = 0;
    fail_fseek = 0;
    fail_fflush = 0;
    fail_ferror = 0;
    fail_rename = 0;
    fail_snprintf = 0;
    fail_errno = 0;
}

static void *tst_malloc(size_t size)
{
    return tst_fail(&cnt_malloc, fail_malloc) ? NULL : malloc(size);
}

static FILE *tst_fopen(const char *path, const char *mode)
{
    return tst_fail(&cnt_fopen, fail_fopen) ? NULL : fopen(path, mode);
}

/**
 * Close the stream in every case, so that a failure reported here does not leak
 * it, and report the failure afterwards.
 */
static int tst_fclose(FILE *stream)
{
    const int rv = fclose(stream);
    return tst_fail(&cnt_fclose, fail_fclose) ? EOF : rv;
}

static size_t tst_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    return tst_fail(&cnt_fwrite, fail_fwrite) ? 0 : fwrite(ptr, size, nmemb, stream);
}

static int tst_fputc(int c, FILE *stream)
{
    return tst_fail(&cnt_fputc, fail_fputc) ? EOF : fputc(c, stream);
}

static int tst_fseek(FILE *stream, long offset, int whence)
{
    return tst_fail(&cnt_fseek, fail_fseek) ? -1 : fseek(stream, offset, whence);
}

static int tst_fflush(FILE *stream)
{
    return tst_fail(&cnt_fflush, fail_fflush) ? EOF : fflush(stream);
}

static int tst_ferror(FILE *stream)
{
    return tst_fail(&cnt_ferror, fail_ferror) ? 1 : ferror(stream);
}

static int tst_rename(const char *from, const char *to)
{
    return tst_fail(&cnt_rename, fail_rename) ? -1 : rename(from, to);
}

/**
 * A negative return reports the encoding error that the tool guards against.
 */
static int tst_snprintf(char *str, size_t size, const char *format, ...)
{
    if (tst_fail(&cnt_snprintf, fail_snprintf))
    {
        return -1;
    }
    va_list args;
    va_start(args, format);
    const int rv = vsnprintf(str, size, format, args);
    va_end(args);
    return rv;
}

// The standard headers are included above, so that the names below are only
// replaced in the code under test, which is included after them.
#define main   vkbin_main
#define malloc tst_malloc
#define fopen  tst_fopen
#define fclose tst_fclose
#define fwrite tst_fwrite
#define fputc  tst_fputc
#define fseek  tst_fseek
#define fflush tst_fflush
#define ferror tst_ferror
#define rename tst_rename
#define snprintf tst_snprintf
#include "../vkbin/vkbin.c" // NOLINT(bugprone-suspicious-include)
#undef snprintf
#undef rename
#undef ferror
#undef fflush
#undef fseek
#undef fputc
#undef fwrite
#undef fclose
#undef fopen
#undef malloc
#undef main

/**
 * Write a file with the given content.
 *
 * @return True on success.
 */
static bool write_file(const char *path, const char *content, size_t len)
{
    FILE *f = fopen(path, "wb");
    if (f == NULL)
    {
        return false;
    }
    const bool ok = (fwrite(content, 1, len, f) == len);
    return ((fclose(f) == 0) && ok);
}

/**
 * Run the tool over the given input, in this process.
 *
 * The arguments are split on the spaces, as a shell would, and the input is
 * written to a file that replaces the standard input. The armed failures are
 * cleared before returning, so that each case only affects its own run.
 *
 * @param args  Command line arguments, without the program name.
 * @param input Content of the standard input.
 *
 * @return The exit status of the tool, or -1 when the case cannot be set up.
 */
static int run_main(const char *args, const char *input)
{
    static char buf[TEST_MAX_ARGS_LEN];
    char *argv[TEST_MAX_ARGS];
    if (strlen(args) >= sizeof(buf))
    {
        return -1;
    }
    memcpy(buf, args, (strlen(args) + 1));
    int argc = 0;
    argv[argc++] = (char *)"vkbin";
    char *tok = strtok(buf, " ");
    while ((tok != NULL) && (argc < (TEST_MAX_ARGS - 1)))
    {
        argv[argc++] = tok;
        tok = strtok(NULL, " ");
    }
    argv[argc] = NULL;
    if (!write_file(TEST_TSV, input, strlen(input)))
    {
        return -1;
    }
    if (freopen(TEST_TSV, "rb", stdin) == NULL)
    {
        return -1;
    }
    const int rv = vkbin_main(argc, argv);
    reset_faults();
    return rv;
}

/**
 * Check the scaled integer conversion.
 */
static int test_parse_scaled(void)
{
    int errors = 0;
    size_t i = 0;
    static const struct
    {
        const char *s;
        uint8_t     decimals;
        int64_t     exp;
    } good[] =
    {
        {"0",                       5,  0},
        {"0.0000000",               5,  0},
        {"1",                       0,  1},
        {"+1.5",                    0,  2},          // ties round away from zero
        {"-1.5",                    0,  -2},
        {"1.4",                     0,  1},
        {"1.234565",                5,  123457},
        {".5",                      1,  5},          // no integer part
        {"5.",                      1,  50},         // no fraction part
        {"1e3",                     0,  1000},
        {"1E-3",                    5,  100},
        {"1e+3",                    0,  1000},
        {"12e-1",                   0,  1},          // 1.2 truncates to 1
        {"1.5e2",                   0,  150},        // the fraction ends at the exponent
        {"0.1234567890123456789",   5,  12346},      // the mantissa holds every digit
        {"0.12345678901234567895",  5,  12346},      // the last digit does not fit
        {"0.123456789012345678955", 5,  12346},      // and neither does the one after it
        {"0.12345678901234567895e14", 5, 1234567890123456790LL}, // and rounds up
        {"1e-19",                   0,  0},          // below half of the smallest unit
        {"5000000000000000000e-19", 0,  1},          // exactly half, rounds away
        {"1e-20",                   0,  0},          // the divisor is wider still
    };
    static const struct
    {
        const char *s;
        size_t      len;
        uint8_t     decimals;
    } bad[] =
    {
        {"",                     0,  5},   // empty field
        {"1",                    1,  19},  // more decimals than the scale holds
        {"abc",                  3,  5},   // not a number
        {"1x",                   2,  5},   // trailing garbage
        {"-",                    1,  5},   // sign without digits
        {".",                    1,  5},   // point without digits
        {"99999999999999999999", 20, 0},   // integer part out of range
        {"1e",                   2,  0},   // exponent without digits
        {"1e+",                  3,  0},   // exponent sign without digits
        {"1e99999",              7,  0},   // exponent past the accumulation cap
        {"1e19",                 4,  0},   // result wider than an int64_t
        {"1.5.2",                5,  0},   // a second point in the fraction
        {"1e5x",                 4,  0},   // exponent followed by a letter
        {"1e5.",                 4,  0},   // exponent followed by a point
        {"999999999999999999",   18, 18},  // scaling overflows an int64_t
    };
    for (i = 0; i < (sizeof(good) / sizeof(good[0])); i++)
    {
        int64_t v = 0;
        if (!parse_scaled(good[i].s, strlen(good[i].s), good[i].decimals, &v))
        {
            (void) fprintf(stderr, "%s (%zu): '%s' was rejected\n", __func__, i, good[i].s);
            ++errors;
            continue;
        }
        if (v != good[i].exp)
        {
            (void) fprintf(stderr, "%s (%zu): '%s' expected %" PRId64 ", got %" PRId64 "\n",
                           __func__, i, good[i].s, good[i].exp, v);
            ++errors;
        }
    }
    for (i = 0; i < (sizeof(bad) / sizeof(bad[0])); i++)
    {
        int64_t v = 0;
        if (parse_scaled(bad[i].s, bad[i].len, bad[i].decimals, &v))
        {
            (void) fprintf(stderr, "%s: '%s' was accepted as %" PRId64 "\n",
                           __func__, bad[i].s, v);
            ++errors;
        }
    }
    return errors;
}

/**
 * Check the case-insensitive comparison and the missing value markers.
 */
static int test_fields(void)
{
    int errors = 0;
    size_t i = 0;
    static const struct
    {
        const char *s;
        const char *lit;
        bool        exp;
    } ieq[] =
    {
        {"",    "",     true},
        {"na",  "NA",   true},
        {"NA",  "NA",   true},
        {"nAx", "NA",   false}, // longer than the literal
        {"nb",  "NA",   false}, // different letter
        {"n{",  "NA",   false}, // character past the letters
        {"n",   "NA",   false}, // shorter than the literal
        {"n/A", "N/A",  true},
    };
    static const struct
    {
        const char *s;
        bool        exp;
    } missing[] =
    {
        {"",     true},
        {".",    true},
        {"NA",   true},
        {"na",   true},
        {"N/A",  true},
        {"NaN",  true},
        {"null", true},
        {"..",   false},
        {"0",    false},
        {"nope", false},
    };
    for (i = 0; i < (sizeof(ieq) / sizeof(ieq[0])); i++)
    {
        if (ascii_ieq(ieq[i].s, strlen(ieq[i].s), ieq[i].lit) != ieq[i].exp)
        {
            (void) fprintf(stderr, "%s (%zu): '%s' against '%s'\n",
                           __func__, i, ieq[i].s, ieq[i].lit);
            ++errors;
        }
    }
    for (i = 0; i < (sizeof(missing) / sizeof(missing[0])); i++)
    {
        if (is_missing(missing[i].s, strlen(missing[i].s)) != missing[i].exp)
        {
            (void) fprintf(stderr, "%s (%zu): '%s'\n", __func__, i, missing[i].s);
            ++errors;
        }
    }
    return errors;
}

/**
 * Check the column specification parser.
 */
static int test_parse_colspec(void)
{
    int errors = 0;
    size_t i = 0;
    static const struct
    {
        const char *arg;
        uint8_t     width;
        uint8_t     decimals;
        bool        na;
        int64_t     offset;
        uint64_t    maxval;
    } good[] =
    {
        {"1:0",        1, 0, false, 0,      UINT64_C(255)},
        {"2:3",        2, 3, false, 0,      UINT64_C(65535)},
        {"4:5:2.0",    4, 5, false, 200000, UINT64_C(4294967295)},
        {"8:0",        8, 0, false, 0,      UINT64_MAX},
        {"4:5:NA",     4, 5, true,  0,      UINT64_C(4294967295)},
        {"4:5:NA:1.5", 4, 5, true,  150000, UINT64_C(4294967295)},
        {"4:5:1.5:na", 4, 5, true,  150000, UINT64_C(4294967295)},
    };
    static const char *bad[] =
    {
        "3:0",                       // width that is not 1, 2, 4 or 8
        "x:0",                       // width that is not a number
        "1",                         // no decimals
        "1:",                        // decimals that are not a number
        "1:19",                      // more decimals than the scale holds
        "1:0x",                      // trailing garbage
        "99999999999999999999999:0", // width out of range
        "1:99999999999999999999999", // decimals out of range
        "1:0:NA:NA",                 // repeated marker
        "1:0:1:2",                   // repeated offset
        "1:0:zz",                    // offset that is not a number
    };
    for (i = 0; i < (sizeof(good) / sizeof(good[0])); i++)
    {
        valcol_t vc;
        memset(&vc, 0, sizeof(vc));
        if (!parse_colspec(good[i].arg, &vc))
        {
            (void) fprintf(stderr, "%s (%zu): '%s' was rejected\n", __func__, i, good[i].arg);
            ++errors;
            continue;
        }
        if ((vc.width != good[i].width) || (vc.decimals != good[i].decimals)
                || (vc.na != good[i].na) || (vc.offset != good[i].offset)
                || (vc.maxval != good[i].maxval))
        {
            (void) fprintf(stderr,
                           "%s (%zu): '%s' gave %u:%u:%" PRId64 "%s max %" PRIu64 "\n",
                           __func__, i, good[i].arg, vc.width, vc.decimals, vc.offset,
                           (vc.na ? ":NA" : ""), vc.maxval);
            ++errors;
        }
    }
    for (i = 0; i < (sizeof(bad) / sizeof(bad[0])); i++)
    {
        valcol_t vc;
        memset(&vc, 0, sizeof(vc));
        if (parse_colspec(bad[i], &vc))
        {
            (void) fprintf(stderr, "%s: '%s' was accepted\n", __func__, bad[i]);
            ++errors;
        }
    }
    return errors;
}

/**
 * Check the little-endian store, the alignment and the field splitting.
 */
static int test_encoding(void)
{
    int errors = 0;
    size_t i = 0;
    static const struct
    {
        uint64_t v;
        uint8_t  width;
        uint8_t  exp[8];
    } stores[] =
    {
        {UINT64_C(0x12),               1, {0x12, 0, 0, 0, 0, 0, 0, 0}},
        {UINT64_C(0x1234),             2, {0x34, 0x12, 0, 0, 0, 0, 0, 0}},
        {UINT64_C(0x12345678),         4, {0x78, 0x56, 0x34, 0x12, 0, 0, 0, 0}},
        {UINT64_C(0x123456789abcdef0), 8, {0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12}},
    };
    static const struct
    {
        uint64_t off;
        uint8_t  align;
        uint64_t exp;
    } aligns[] =
    {
        {0,  8, 0},
        {8,  8, 8},
        {9,  8, 16},
        {17, 4, 20},
        {3,  1, 3},
    };
    for (i = 0; i < (sizeof(stores) / sizeof(stores[0])); i++)
    {
        uint8_t b[8] = {0};
        store_uint_le(b, stores[i].v, stores[i].width);
        if (memcmp(b, stores[i].exp, stores[i].width) != 0)
        {
            (void) fprintf(stderr, "%s (%zu): wrong bytes for width %u\n",
                           __func__, i, stores[i].width);
            ++errors;
        }
    }
    for (i = 0; i < (sizeof(aligns) / sizeof(aligns[0])); i++)
    {
        const uint64_t got = align_up(aligns[i].off, aligns[i].align);
        if (got != aligns[i].exp)
        {
            (void) fprintf(stderr, "%s (%zu): expected %" PRIu64 ", got %" PRIu64 "\n",
                           __func__, i, aligns[i].exp, got);
            ++errors;
        }
    }
    const char *fs[4] = {NULL};
    size_t fl[4] = {0};
    const char *line = "a\tbb\t\tdddd";
    size_t n = split_fields(line, strlen(line), fs, fl, 4);
    if ((n != 4) || (fl[0] != 1) || (fl[1] != 2) || (fl[2] != 0) || (fl[3] != 4))
    {
        (void) fprintf(stderr, "%s: unexpected fields, got %zu\n", __func__, n);
        ++errors;
    }
    // A line with more fields than the capacity stops at the capacity.
    n = split_fields(line, strlen(line), fs, fl, 2);
    if ((n != 2) || (fs[1] != (line + 2)))
    {
        (void) fprintf(stderr, "%s: expected 2 capped fields, got %zu\n", __func__, n);
        ++errors;
    }
    return errors;
}

/**
 * Check the buffered writers and the spool files.
 */
static int test_streams(void)
{
    int errors = 0;
    static uint8_t mem[VKBIN_WBUFSIZE];
    FILE *f = fopen(TEST_TMP, "w+b");
    if (f == NULL)
    {
        (void) fprintf(stderr, "%s: cannot create '%s'\n", __func__, TEST_TMP);
        return 1;
    }
    wbuf_t w = { f, mem, 0 };
    // Nothing pending is not a failure.
    if (!wbuf_flush(&w))
    {
        (void) fprintf(stderr, "%s: the empty flush failed\n", __func__);
        ++errors;
    }
    // A value that does not fit hands the pending bytes to the stream first.
    w.len = (VKBIN_WBUFSIZE - 4);
    if (!wbuf_put(&w, UINT64_C(0x0102030405060708), 8) || (w.len != 8))
    {
        (void) fprintf(stderr, "%s: the flushing put failed\n", __func__);
        ++errors;
    }
    w.len = VKBIN_WBUFSIZE;
    reset_faults();
    fail_fwrite = 1;
    if (wbuf_put(&w, 0, 1))
    {
        (void) fprintf(stderr, "%s: a failed flush was not reported\n", __func__);
        ++errors;
    }
    reset_faults();
    if (!write_uint_le(f, UINT64_C(0x0102030405060708), 8))
    {
        (void) fprintf(stderr, "%s: the header write failed\n", __func__);
        ++errors;
    }
    reset_faults();
    fail_fwrite = 1;
    if (write_uint_le(f, 0, 8))
    {
        (void) fprintf(stderr, "%s: a failed header write was not reported\n", __func__);
        ++errors;
    }
    reset_faults();
    (void) fclose(f);
    (void) remove(TEST_TMP);
    // The spool of a path that cannot hold the suffix, and of a directory that
    // does not exist.
    static char longpath[(VKBIN_MAX_PATH + 16)];
    memset(longpath, 'x', (sizeof(longpath) - 1));
    longpath[sizeof(longpath) - 1] = '\0';
    errno = 0;
    FILE *sp = open_spool(longpath, 0);
    if ((sp != NULL) || (errno != ENAMETOOLONG))
    {
        (void) fprintf(stderr, "%s: an over-long spool path was accepted\n", __func__);
        ++errors;
        if (sp != NULL)
        {
            (void) fclose(sp);
        }
    }
    reset_faults();
    fail_snprintf = 1;
    sp = open_spool(TEST_TMP, 0);
    if (sp != NULL)
    {
        (void) fprintf(stderr, "%s: a spool path that cannot be built was accepted\n", __func__);
        ++errors;
        (void) fclose(sp);
    }
    reset_faults();
    sp = open_spool("test_vkbin_unit_missing_dir/spool", 0);
    if (sp != NULL)
    {
        (void) fprintf(stderr, "%s: a spool was created in a missing directory\n", __func__);
        ++errors;
        (void) fclose(sp);
    }
    sp = open_spool(TEST_TMP, 0);
    if (sp == NULL)
    {
        (void) fprintf(stderr, "%s: the spool was not created\n", __func__);
        ++errors;
    }
    else
    {
        (void) fclose(sp);
    }
    return errors;
}

/**
 * Check the end of input detection and the line skipping.
 *
 * Each case starts from a known position, because after a read that may have
 * failed the position of a stream is only defined once it is set again.
 */
static int test_input(void)
{
    int errors = 0;
    if (!write_file(TEST_TSV, "ab\ncd", 5) || !write_file(TEST_TMP, "", 0))
    {
        (void) fprintf(stderr, "%s: cannot create the input files\n", __func__);
        return 1;
    }
    FILE *e = fopen(TEST_TMP, "rb");
    if (e == NULL)
    {
        (void) fprintf(stderr, "%s: cannot open '%s'\n", __func__, TEST_TMP);
        return 1;
    }
    const bool ended = at_end(e);
    (void) fclose(e);
    FILE *f = fopen(TEST_TSV, "rb");
    if (f == NULL)
    {
        (void) fprintf(stderr, "%s: cannot open '%s'\n", __func__, TEST_TSV);
        return 1;
    }
    if (!ended || at_end(f))
    {
        (void) fprintf(stderr, "%s: the end of the input was misreported\n", __func__);
        ++errors;
    }
    // A line is discarded up to its terminator, and the last one up to the end
    // of the input.
    if ((fseek(f, 0, SEEK_SET) != 0) || !discard_line(f) || (ftell(f) != 3))
    {
        (void) fprintf(stderr, "%s: the first line was not discarded\n", __func__);
        ++errors;
    }
    if ((fseek(f, 3, SEEK_SET) != 0) || !discard_line(f) || (ftell(f) != 5))
    {
        (void) fprintf(stderr, "%s: the last line was not discarded\n", __func__);
        ++errors;
    }
    // A failing read is reported instead of the end of the input.
    reset_faults();
    fail_ferror = 1;
    if ((fseek(f, 0, SEEK_SET) != 0) || discard_line(f))
    {
        (void) fprintf(stderr, "%s: a read error was not reported\n", __func__);
        ++errors;
    }
    reset_faults();
    (void) fclose(f);
    (void) remove(TEST_TSV);
    (void) remove(TEST_TMP);
    return errors;
}

/**
 * Check that a line is refused when its columns cannot be spooled.
 */
static int test_process_line(void)
{
    int errors = 0;
    static uint8_t vkmem[VKBIN_WBUFSIZE];
    static uint8_t colmem[VKBIN_WBUFSIZE];
    const char *fs[8] = {NULL};
    size_t fl[8] = {0};
    const char *line = "chr1\t100\tA\tC\t1.5";
    const size_t len = strlen(line);
    valcol_t col;
    memset(&col, 0, sizeof(col));
    if (!parse_colspec("4:5", &col))
    {
        (void) fprintf(stderr, "%s: the column specification was rejected\n", __func__);
        return 1;
    }
    FILE *f = fopen(TEST_TMP, "w+b");
    if (f == NULL)
    {
        (void) fprintf(stderr, "%s: cannot create '%s'\n", __func__, TEST_TMP);
        return 1;
    }
    col.spool.f = f;
    col.spool.buf = colmem;
    wbuf_t vkcol = { f, vkmem, VKBIN_WBUFSIZE };
    uint64_t prevkey = 0;
    // A full buffer makes the next value reach the stream, which fails.
    reset_faults();
    fail_fwrite = 1;
    if (process_line(line, len, 1, &col, 1, &vkcol, &prevkey, fs, fl))
    {
        (void) fprintf(stderr, "%s: a failed key column write was not reported\n", __func__);
        ++errors;
    }
    reset_faults();
    vkcol.len = 0;
    col.spool.len = VKBIN_WBUFSIZE;
    prevkey = 0;
    reset_faults();
    fail_fwrite = 1;
    if (process_line(line, len, 1, &col, 1, &vkcol, &prevkey, fs, fl))
    {
        (void) fprintf(stderr, "%s: a failed value column write was not reported\n", __func__);
        ++errors;
    }
    reset_faults();
    (void) fclose(f);
    (void) remove(TEST_TMP);
    return errors;
}

/**
 * Number of rows of the finalize test fixture.
 */
enum { TEST_FINAL_ROWS = 2 };

/**
 * Size of the header of the finalize test fixture, which has three columns.
 */
enum { TEST_FINAL_HDRLEN = 48 };

/**
 * Run finalize over a fixture of a one byte and an eight byte column, with the
 * given failure armed.
 *
 * The value columns are spooled as the tool does, and the output already holds
 * the header placeholder and the VariantKey column, so that the value columns
 * are appended at the offsets the header will declare.
 *
 * @param arm Failure to arm, or NULL for a run that must succeed.
 * @param at  Call number that must fail.
 *
 * @return True when finalize returned what the case expects.
 */
static bool run_finalize(int *arm, int at)
{
    static uint8_t buf1[VKBIN_WBUFSIZE];
    static uint8_t buf8[VKBIN_WBUFSIZE];
    valcol_t cols[2];
    memset(cols, 0, sizeof(cols));
    cols[0].width = 1;
    cols[0].spool.buf = buf1;
    cols[1].width = 8;
    cols[1].spool.buf = buf8;
    FILE *out = fopen(TEST_TMP, "w+b");
    cols[0].spool.f = fopen(TEST_TMP ".1", "w+b");
    cols[1].spool.f = fopen(TEST_TMP ".8", "w+b");
    bool ok = ((out != NULL) && (cols[0].spool.f != NULL) && (cols[1].spool.f != NULL));
    if (ok)
    {
        uint64_t i = 0;
        for (i = 0; i < (TEST_FINAL_HDRLEN + (TEST_FINAL_ROWS * 8)); i++)
        {
            ok = ok && (fputc(0, out) != EOF);
        }
        for (i = 0; i < TEST_FINAL_ROWS; i++)
        {
            ok = ok && wbuf_put(&cols[0].spool, i, 1) && wbuf_put(&cols[1].spool, i, 8);
        }
        ok = ok && wbuf_flush(&cols[0].spool) && wbuf_flush(&cols[1].spool);
    }
    if (ok)
    {
        reset_faults();
        if (arm != NULL)
        {
            *arm = at;
        }
        ok = (finalize(out, cols, 2, TEST_FINAL_ROWS, TEST_FINAL_HDRLEN) == (arm == NULL));
        reset_faults();
    }
    if (out != NULL)
    {
        (void) fclose(out);
    }
    if (cols[0].spool.f != NULL)
    {
        (void) fclose(cols[0].spool.f);
    }
    if (cols[1].spool.f != NULL)
    {
        (void) fclose(cols[1].spool.f);
    }
    (void) remove(TEST_TMP);
    (void) remove(TEST_TMP ".1");
    (void) remove(TEST_TMP ".8");
    return ok;
}

/**
 * Check that finalize reports every failure of the output it writes.
 */
static int test_finalize(void)
{
    int errors = 0;
    size_t i = 0;
    static const struct
    {
        int        *arm;
        int         at;
        const char *desc;
    } cases[] =
    {
        {NULL,         0, "a complete output"},
        {&fail_fflush, 1, "the spool cannot be flushed"},
        {&fail_fseek,  1, "the spool cannot be rewound"},
        {&fail_fwrite, 1, "the spool cannot be copied"},
        {&fail_ferror, 1, "the spool cannot be read"},
        {&fail_fputc,  1, "the column cannot be aligned"},
        {&fail_fseek,  3, "the output cannot be rewound"},
        {&fail_fwrite, 3, "the magic number cannot be written"},
        {&fail_fwrite, 4, "the column sizes cannot be written"},
        {&fail_fwrite, 5, "the row count cannot be written"},
        {&fail_fwrite, 6, "the column index cannot be written"},
    };
    for (i = 0; i < (sizeof(cases) / sizeof(cases[0])); i++)
    {
        if (!run_finalize(cases[i].arm, cases[i].at))
        {
            (void) fprintf(stderr, "%s (%zu): unexpected result when %s\n",
                           __func__, i, cases[i].desc);
            ++errors;
        }
    }
    return errors;
}

/**
 * Check a converted file against the expected column sizes and values.
 *
 * @param ctbytes Expected item size of each column.
 * @param nrows   Expected number of rows.
 * @param ncols   Expected number of columns.
 *
 * @return The mapped file, with a NULL source when a check failed.
 */
static mmfile_t check_output(const uint8_t *ctbytes, uint64_t nrows, uint8_t ncols)
{
    mmfile_t mf = {0};
    mmap_binfile(TEST_BIN, &mf);
    if (mf.src == (uint8_t *)MAP_FAILED)
    {
        (void) fprintf(stderr, "%s: cannot map '%s'\n", __func__, TEST_BIN);
        mf.src = NULL;
        return mf;
    }
    if ((mf.ncols != ncols) || (mf.nrows != nrows))
    {
        (void) fprintf(stderr, "%s: expected %u columns and %" PRIu64 " rows, got %u and %" PRIu64 "\n",
                       __func__, ncols, nrows, mf.ncols, mf.nrows);
        (void) munmap_binfile(&mf);
        mf.src = NULL;
        return mf;
    }
    uint8_t i = 0;
    for (i = 0; i < ncols; i++)
    {
        if ((mf.ctbytes[i] != ctbytes[i]) || ((mf.index[i] % mf.ctbytes[i]) != 0))
        {
            (void) fprintf(stderr, "%s: column %u has size %u at offset %" PRIu64 "\n",
                           __func__, i, mf.ctbytes[i], mf.index[i]);
            (void) munmap_binfile(&mf);
            mf.src = NULL;
            return mf;
        }
    }
    return mf;
}

/**
 * Check the conversion of the column widths the tool accepts.
 */
static int test_widths(void)
{
    int errors = 0;
    static const uint8_t ctbytes[4] = {8, 1, 2, 8};
    const char *input =
        "#CHROM\tPOS\tREF\tALT\tA\tB\tC\n"
        "chr1\t100\tA\tC\t1\t300\t12345678901\n"
        "chr1\t200\tG\tT\t255\t65534\t0\n";
    if (run_main("-o " TEST_BIN " 1:0 2:0 8:0", input) != 0)
    {
        (void) fprintf(stderr, "%s: the conversion failed\n", __func__);
        return 1;
    }
    mmfile_t mf = check_output(ctbytes, 2, 4);
    if (mf.src == NULL)
    {
        return 1;
    }
    const uint8_t *a = get_src_offset_uint8_t(mf.src, mf.index[1]);
    const uint16_t *b = get_src_offset_uint16_t(mf.src, mf.index[2]);
    const uint64_t *c = get_src_offset_uint64_t(mf.src, mf.index[3]);
    if ((a[0] != 1) || (a[1] != 255) || (b[0] != 300) || (b[1] != 65534)
            || (c[0] != UINT64_C(12345678901)) || (c[1] != 0))
    {
        (void) fprintf(stderr, "%s: wrong stored values\n", __func__);
        ++errors;
    }
    (void) munmap_binfile(&mf);
    (void) remove(TEST_BIN);
    return errors;
}

/**
 * Check the missing value marker, the exponents and the line endings.
 */
static int test_values(void)
{
    int errors = 0;
    static const uint8_t ctbytes[2] = {8, 1};
    const char *missing =
        "#CHROM\tPOS\tREF\tALT\tV\n"
        "chr1\t100\tA\tC\t.\n"
        "chr1\t200\tA\tC\tNA\n"
        "chr1\t300\tA\tC\tn/a\n"
        "chr1\t400\tA\tC\tNaN\n"
        "chr1\t500\tA\tC\tnull\n"
        "chr1\t600\tA\tC\t\n"
        "chr1\t700\tA\tC\t1.5\n";
    if (run_main("-o " TEST_BIN " 1:0:NA", missing) != 0)
    {
        (void) fprintf(stderr, "%s: the conversion of the missing values failed\n", __func__);
        return 1;
    }
    mmfile_t mf = check_output(ctbytes, 7, 2);
    if (mf.src == NULL)
    {
        return 1;
    }
    const uint8_t *v = get_src_offset_uint8_t(mf.src, mf.index[1]);
    uint64_t i = 0;
    for (i = 0; i < 6; i++)
    {
        if (v[i] != 255)
        {
            (void) fprintf(stderr, "%s: row %" PRIu64 " is not marked as missing\n", __func__, i);
            ++errors;
        }
    }
    if (v[6] != 2)
    {
        (void) fprintf(stderr, "%s: expected 2, got %u\n", __func__, v[6]);
        ++errors;
    }
    (void) munmap_binfile(&mf);
    (void) remove(TEST_BIN);
    // Two header lines, a blank line, a carriage return and both exponent signs.
    static const uint8_t ctbytes2[2] = {8, 4};
    const char *mixed =
        "# generated file\n"
        "#CHROM\tPOS\tREF\tALT\tV\n"
        "chr1\t100\tA\tC\t1e2\r\n"
        "\n"
        "chr1\t200\tA\tC\t2.5e-1\n";
    if (run_main("-s 2 -o " TEST_BIN " 4:5", mixed) != 0)
    {
        (void) fprintf(stderr, "%s: the conversion of the mixed input failed\n", __func__);
        return (errors + 1);
    }
    mf = check_output(ctbytes2, 2, 2);
    if (mf.src == NULL)
    {
        return (errors + 1);
    }
    const uint32_t *w = get_src_offset_uint32_t(mf.src, mf.index[1]);
    if ((w[0] != UINT32_C(10000000)) || (w[1] != UINT32_C(25000)))
    {
        (void) fprintf(stderr, "%s: expected 10000000 and 25000, got %" PRIu32 " and %" PRIu32 "\n",
                       __func__, w[0], w[1]);
        ++errors;
    }
    (void) munmap_binfile(&mf);
    (void) remove(TEST_BIN);
    return errors;
}

/**
 * Check that the tool refuses the input it cannot encode.
 */
static int test_input_errors(void)
{
    int errors = 0;
    size_t i = 0;
    static const struct
    {
        const char *args;
        const char *input;
        const char *desc;
    } cases[] =
    {
        {
            "-o " TEST_BIN " 1:0", "#H\nchr1\t100\tA\n",
            "fewer columns than the specifications"
        },
        {
            "-o " TEST_BIN " 1:0", "#H\nchr1\tx\tA\tC\t1\n",
            "a position that is not a number"
        },
        {
            "-o " TEST_BIN " 1:0", "#H\nchr1\t\tA\tC\t1\n",
            "an empty position"
        },
        {
            "-o " TEST_BIN " 1:0", "#H\nchr1\t0\tA\tC\t1\n",
            "a position below one"
        },
        {
            "-o " TEST_BIN " 1:0", "#H\nchr1\t268435457\tA\tC\t1\n",
            "a position past the 28 bit range"
        },
        {
            "-o " TEST_BIN " 1:0", "#H\nchr1\t99999999999\tA\tC\t1\n",
            "a position with too many digits"
        },
        {
            "-o " TEST_BIN " 1:0", "#H\nchr1\t100\tA\tC\tx\n",
            "a value that is not a number"
        },
        {
            "-o " TEST_BIN " 1:0", "#H\nchr1\t100\tA\tC\t300\n",
            "a value wider than the column"
        },
        {
            "-o " TEST_BIN " 1:0", "#H\nchr1\t100\tA\tC\t-1\n",
            "a negative value without an offset"
        },
        {
            "-o " TEST_BIN " 8:0:900000000000000000", "#H\nchr1\t100\tA\tC\t9e18\n",
            "a positive value that overflows the offset"
        },
        {
            "-o " TEST_BIN " 8:0:-900000000000000000", "#H\nchr1\t100\tA\tC\t-9e18\n",
            "a negative value that overflows the offset"
        },
        {
            "-o " TEST_BIN " 1:0:NA", "#H\nchr1\t100\tA\tC\t255\n",
            "a value that collides with the missing value marker"
        },
        {
            "-o " TEST_BIN " 1:0", "#H\nchr2\t100\tA\tC\t1\nchr1\t100\tA\tC\t1\n",
            "an input that is not sorted"
        },
        {
            "-o " TEST_BIN " 1:0", "#H\n",
            "an input without a data row"
        },
    };
    for (i = 0; i < (sizeof(cases) / sizeof(cases[0])); i++)
    {
        if (run_main(cases[i].args, cases[i].input) == 0)
        {
            (void) fprintf(stderr, "%s (%zu): accepted %s\n", __func__, i, cases[i].desc);
            ++errors;
            (void) remove(TEST_BIN);
        }
    }
    return errors;
}

/**
 * Check how an input line that fills the read buffer is handled.
 */
static int test_long_lines(void)
{
    int errors = 0;
    static char input[(VKBIN_MAX_LINE * 3)];
    // A data line that does not fit in the buffer is an error, while the same
    // line among the skipped ones is discarded.
    int n = snprintf(input, sizeof(input), "#H\nchr1\t100\tA\t");
    memset((input + n), 'A', VKBIN_MAX_LINE);
    (void) snprintf((input + n + VKBIN_MAX_LINE), (sizeof(input) - (size_t)n - VKBIN_MAX_LINE), "\t1\n");
    if (run_main("-o " TEST_BIN " 1:0", input) == 0)
    {
        (void) fprintf(stderr, "%s: an over-long data line was accepted\n", __func__);
        ++errors;
        (void) remove(TEST_BIN);
    }
    static char skipped[(VKBIN_MAX_LINE * 3)];
    memset(skipped, 'H', VKBIN_MAX_LINE);
    (void) snprintf((skipped + VKBIN_MAX_LINE), (sizeof(skipped) - VKBIN_MAX_LINE), "\nchr1\t100\tA\tC\t1\n");
    if (run_main("-o " TEST_BIN " 1:0", skipped) != 0)
    {
        (void) fprintf(stderr, "%s: an over-long header line was not discarded\n", __func__);
        ++errors;
    }
    (void) remove(TEST_BIN);
    // The same input with the read of the discarded remainder failing.
    reset_faults();
    fail_ferror = 1;
    if (run_main("-o " TEST_BIN " 1:0", skipped) == 0)
    {
        (void) fprintf(stderr, "%s: a read error was not reported\n", __func__);
        ++errors;
        (void) remove(TEST_BIN);
    }
    // A line that fills the buffer is complete when it ends with a terminator,
    // and also when it is the last one of the input.
    static const uint8_t ctbytes[2] = {8, 1};
    static char full[(VKBIN_MAX_LINE * 3)];
    n = snprintf(full, sizeof(full), "#H\nchr1\t100\tA\t");
    size_t altlen = (VKBIN_MAX_LINE - 1 - strlen("chr1\t100\tA\t") - strlen("\t1\n"));
    memset((full + n), 'A', altlen);
    n += (int)altlen;
    n += snprintf((full + n), (sizeof(full) - (size_t)n), "\t1\nchr1\t200\tA\t");
    altlen = (VKBIN_MAX_LINE - 1 - strlen("chr1\t200\tA\t") - strlen("\t2"));
    memset((full + n), 'C', altlen);
    n += (int)altlen;
    (void) snprintf((full + n), (sizeof(full) - (size_t)n), "\t2");
    if (run_main("-o " TEST_BIN " 1:0", full) != 0)
    {
        (void) fprintf(stderr, "%s: a full line was rejected\n", __func__);
        return (errors + 1);
    }
    mmfile_t mf = check_output(ctbytes, 2, 2);
    if (mf.src == NULL)
    {
        return (errors + 1);
    }
    (void) munmap_binfile(&mf);
    (void) remove(TEST_BIN);
    return errors;
}

/**
 * Check that every failure of the library calls the tool makes is reported.
 */
static int test_faults(void)
{
    int errors = 0;
    size_t i = 0;
    const char *input = "#H\nchr1\t100\tA\tC\t1\nchr1\t200\tA\tC\t2\n";
    static const struct
    {
        int        *arm;
        int         at;
        int         err;
        const char *desc;
    } cases[] =
    {
        {&fail_snprintf, 1, 0,    "the temporary path cannot be built"},
        {&fail_malloc, 1, 0,      "the write buffers cannot be allocated"},
        {&fail_fopen,  1, EACCES, "the output cannot be created"},
        {&fail_fopen,  2, EACCES, "the spool cannot be created"},
        {&fail_fputc,  1, EACCES, "the header cannot be written"},
        {&fail_ferror, 1, 0,      "the input cannot be read"},
        {&fail_fwrite, 1, 0,      "the key column cannot be written"},
        {&fail_fwrite, 2, EACCES, "the value column cannot be written"},
        {&fail_fwrite, 3, EACCES, "the output cannot be completed"},
        {&fail_fclose, 2, EACCES, "the output cannot be closed"},
        {&fail_rename, 1, EACCES, "the output cannot be renamed"},
    };
    for (i = 0; i < (sizeof(cases) / sizeof(cases[0])); i++)
    {
        reset_faults();
        *(cases[i].arm) = cases[i].at;
        fail_errno = cases[i].err;
        if (run_main("-o " TEST_BIN " 1:0", input) == 0)
        {
            (void) fprintf(stderr, "%s (%zu): no failure reported when %s\n",
                           __func__, i, cases[i].desc);
            ++errors;
        }
        if (remove(TEST_BIN) == 0)
        {
            (void) fprintf(stderr, "%s (%zu): an output was left behind when %s\n",
                           __func__, i, cases[i].desc);
            ++errors;
        }
    }
    return errors;
}

int main(void)
{
    int errors = 0;

    errors += test_parse_scaled();
    errors += test_fields();
    errors += test_parse_colspec();
    errors += test_encoding();
    errors += test_streams();
    errors += test_input();
    errors += test_process_line();
    errors += test_finalize();
    errors += test_widths();
    errors += test_values();
    errors += test_input_errors();
    errors += test_long_lines();
    errors += test_faults();

    (void) remove(TEST_TSV);

    return errors;
}
