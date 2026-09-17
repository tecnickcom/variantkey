// VariantKey Binary Lookup Table Builder Command Line Application
//
// vkbin.c
//
// @category   Tools
// @author     Nicola Asuni <info@tecnick.com>
// @link       https://github.com/tecnickcom/variantkey
// @license    MIT [LICENSE](https://raw.githubusercontent.com/tecnickcom/variantkey/main/LICENSE)

// Use 64 bit file offsets, so that spool and output files larger than 2 GiB can
// be written where off_t would otherwise be 32 bit.
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64 // NOLINT(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
#endif

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../src/variantkey/binsearch.h"
#include "../src/variantkey/variantkey.h"

#ifndef VERSION
#define VERSION "0.0.0-0"
#endif

//!< Highest position representable in the 28 bit POS field
enum { VK_MAX_POS = 0x0FFFFFFF };

//!< Maximum number of value columns. MAXCOLS is the largest column count the
//!< one byte field of the BINSRC1 header can hold, and one of those columns is
//!< the VariantKey.
enum { VKBIN_MAX_VALCOLS = (MAXCOLS - 1) };

//!< Size of the input and output stream buffers
enum { VKBIN_BUFSIZE = (1 << 20) };

//!< Size of the write buffer of a single column
enum { VKBIN_WBUFSIZE = (1 << 16) };

//!< Maximum accepted input line length
enum { VKBIN_MAX_LINE = 4096 };

//!< Maximum length of an output or spool file path
enum { VKBIN_MAX_PATH = 4096 };

//!< Maximum number of decimal places accepted in a column specification
enum { VKBIN_MAX_DECIMALS = 18 };

/**
 * @brief Buffered writer of a single column.
 *
 * The values are collected in memory and written to the stream in blocks.
 */
typedef struct wbuf_t
{
    FILE    *f;    //!< Destination stream.
    uint8_t *buf;  //!< Buffer holding the pending bytes.
    size_t  len;   //!< Number of pending bytes.
} wbuf_t;

/**
 * @brief Specification of one value column, parsed from a
 * WIDTH:DECIMALS[:OFFSET][:NA] command line argument.
 */
typedef struct valcol_t
{
    uint8_t  width;     //!< Size of the stored integer in bytes: 1, 2, 4 or 8.
    uint8_t  decimals;  //!< Number of decimal places preserved by the scaling.
    bool     na;        //!< True when "maxval" is reserved for a missing field.
    int64_t  offset;    //!< Value added before storing, already scaled.
    uint64_t maxval;    //!< Largest value that fits in "width" bytes.
    wbuf_t   spool;     //!< Spool file holding the column while the input is read.
} valcol_t;

/**
 * @brief Powers of ten used to scale a decimal string into an integer,
 * up to the largest one that fits in an int64_t.
 */
static const int64_t POW10[VKBIN_MAX_DECIMALS + 1] =
{
    1LL, 10LL, 100LL, 1000LL, 10000LL, 100000LL, 1000000LL, 10000000LL,
    100000000LL, 1000000000LL, 10000000000LL, 100000000000LL,
    1000000000000LL, 10000000000000LL, 100000000000000LL,
    1000000000000000LL, 10000000000000000LL, 100000000000000000LL,
    1000000000000000000LL,
};

/**
 * @brief Stores an unsigned value of the given width in little-endian order.
 *
 * BINSRC1 values are little-endian regardless of the byte order of the machine.
 *
 * @param p     Destination, with at least "width" bytes available.
 * @param v     Value to store.
 * @param width Number of bytes to store: 1, 2, 4 or 8.
 */
static void store_uint_le(uint8_t *p, uint64_t v, uint8_t width)
{
    switch (width)
    {
    case 1:
    {
        const uint8_t t = (uint8_t)v;
        memcpy(p, &t, sizeof(t));
        break;
    }
    case 2:
    {
        uint16_t t = (uint16_t)v;
#ifdef BINSEARCH_BIG_ENDIAN
        t = binsearch_bswap16(t);
#endif
        memcpy(p, &t, sizeof(t));
        break;
    }
    case 4:
    {
        uint32_t t = (uint32_t)v;
#ifdef BINSEARCH_BIG_ENDIAN
        t = binsearch_bswap32(t);
#endif
        memcpy(p, &t, sizeof(t));
        break;
    }
    default:
    {
#ifdef BINSEARCH_BIG_ENDIAN
        v = binsearch_bswap64(v);
#endif
        memcpy(p, &v, sizeof(v));
        break;
    }
    }
}

/**
 * @brief Writes the pending bytes of a buffered writer to its stream.
 *
 * @param w Buffered writer.
 *
 * @return True on success.
 */
static bool wbuf_flush(wbuf_t *w)
{
    const size_t n = w->len;
    if (n == 0)
    {
        return true;
    }
    w->len = 0;
    return (fwrite(w->buf, 1, n, w->f) == n);
}

/**
 * @brief Appends a value to a buffered writer.
 *
 * @param w     Buffered writer.
 * @param v     Value to store.
 * @param width Number of bytes to store: 1, 2, 4 or 8.
 *
 * @return True on success.
 */
static bool wbuf_put(wbuf_t *w, uint64_t v, uint8_t width)
{
    if ((w->len + width) > VKBIN_WBUFSIZE)
    {
        if (!wbuf_flush(w))
        {
            return false;
        }
    }
    store_uint_le((w->buf + w->len), v, width);
    w->len += width;
    return true;
}

/**
 * @brief Converts a decimal string into an integer scaled by 10^decimals.
 *
 * The conversion is done in integer arithmetic, so the result is exact for
 * every input the scale can represent. Digits beyond the scale are rounded
 * half away from zero.
 *
 * Accepts an optional sign, an optional integer part, an optional fraction
 * part and an optional "e" exponent, provided at least one digit is present.
 *
 * @param s        Start of the field, not NUL terminated.
 * @param len      Length of the field.
 * @param decimals Number of decimal places to preserve.
 * @param out      Scaled result. Set only on success.
 *
 * @return True on success, false on a malformed field or on overflow.
 */
static bool parse_scaled(const char *s, size_t len, uint8_t decimals, int64_t *out)
{
    if ((len == 0) || (decimals > VKBIN_MAX_DECIMALS))
    {
        return false;
    }
    size_t i = 0;
    bool neg = false;
    if ((s[0] == '-') || (s[0] == '+'))
    {
        neg = (s[0] == '-');
        i = 1;
    }
    // All the digits go into a single mantissa paired with a power of ten, so
    // that the fraction part and the exponent share the same final scaling.
    const int64_t manthigh = ((INT64_MAX - 9) / 10);
    int64_t mant = 0;
    int64_t fdigits = 0;
    size_t digits = 0;
    bool roundup = false;
    while ((i < len) && (s[i] >= '0') && (s[i] <= '9'))
    {
        if (mant > manthigh)
        {
            return false; // the integer part alone is out of range
        }
        mant = (mant * 10) + (s[i] - '0');
        ++i;
        ++digits;
    }
    if ((i < len) && (s[i] == '.'))
    {
        bool closed = false;
        ++i;
        while ((i < len) && (s[i] >= '0') && (s[i] <= '9'))
        {
            if (!closed && (mant > manthigh))
            {
                // The first digit the mantissa cannot hold decides the
                // rounding: ties round away from zero, so the digits after it
                // cannot change the result.
                closed = true;
                roundup = (s[i] >= '5');
            }
            if (!closed)
            {
                mant = (mant * 10) + (s[i] - '0');
                ++fdigits;
            }
            ++i;
            ++digits;
        }
    }
    int64_t exp = 0;
    if ((i < len) && ((s[i] == 'e') || (s[i] == 'E')))
    {
        ++i;
        bool eneg = false;
        if ((i < len) && ((s[i] == '-') || (s[i] == '+')))
        {
            eneg = (s[i] == '-');
            ++i;
        }
        size_t edigits = 0;
        while ((i < len) && (s[i] >= '0') && (s[i] <= '9'))
        {
            // Anything past the cap is out of range, so the accumulation
            // stops instead of overflowing.
            if (exp < 1000)
            {
                exp = (exp * 10) + (s[i] - '0');
            }
            ++i;
            ++edigits;
        }
        if (edigits == 0)
        {
            return false;
        }
        exp = eneg ? -exp : exp;
    }
    if ((i != len) || (digits == 0))
    {
        return false; // trailing garbage, or no digit at all
    }
    int64_t v = 0;
    const int64_t shift = ((int64_t)decimals - fdigits + exp);
    if (mant == 0)
    {
        v = 0;
    }
    else if (shift >= 0)
    {
        // Nothing is discarded below, so a digit the mantissa could not hold
        // decides the last digit of the result.
        mant += (roundup ? 1 : 0); // the guard above leaves a unit of headroom
        if (shift > VKBIN_MAX_DECIMALS)
        {
            return false; // the result cannot fit in an int64_t
        }
        const int64_t scale = POW10[shift];
        if (mant > (INT64_MAX / scale))
        {
            return false;
        }
        v = (mant * scale);
    }
    else if (-shift <= VKBIN_MAX_DECIMALS)
    {
        // The division discards more than the mantissa did, so the digits it
        // could not hold cannot change the rounded result.
        const int64_t scale = POW10[-shift];
        const int64_t q = (mant / scale);
        v = (((mant - (q * scale)) >= (scale / 2)) ? (q + 1) : q);
    }
    else
    {
        // The divisor is wider than any mantissa, so the result is zero unless
        // the mantissa reaches half of it.
        v = (((-shift == (VKBIN_MAX_DECIMALS + 1))
              && (mant >= (POW10[VKBIN_MAX_DECIMALS] * 5))) ? 1 : 0);
    }
    *out = (neg ? -v : v);
    return true;
}

/**
 * @brief Compares a field with a NUL terminated literal, ignoring the case of
 * the ASCII letters.
 *
 * strcasecmp is POSIX and locale-aware, so the comparison is spelled out.
 *
 * @param s   Start of the field, not NUL terminated.
 * @param len Length of the field.
 * @param lit Literal to compare with, in upper case.
 *
 * @return True when the two match.
 */
static bool ascii_ieq(const char *s, size_t len, const char *lit)
{
    size_t i = 0;
    for (i = 0; i < len; i++)
    {
        if (lit[i] == '\0')
        {
            return false;
        }
        char c = s[i];
        if ((c >= 'a') && (c <= 'z'))
        {
            c = (char)(c - ('a' - 'A'));
        }
        if (c != lit[i])
        {
            return false;
        }
    }
    return (lit[len] == '\0');
}

/**
 * @brief Tells whether a value field marks a missing value.
 *
 * @param s   Start of the field, not NUL terminated.
 * @param len Length of the field.
 *
 * @return True for an empty field or for one of the usual markers.
 */
static bool is_missing(const char *s, size_t len)
{
    if ((len == 0) || ((len == 1) && (s[0] == '.')))
    {
        return true;
    }
    return (ascii_ieq(s, len, "NA") || ascii_ieq(s, len, "N/A")
            || ascii_ieq(s, len, "NAN") || ascii_ieq(s, len, "NULL"));
}

/**
 * @brief Parses a WIDTH:DECIMALS[:OFFSET][:NA] column specification.
 *
 * OFFSET and NA follow DECIMALS in any order and each may appear once.
 *
 * @param arg Specification string.
 * @param vc  Column to fill in.
 *
 * @return True on success.
 */
static bool parse_colspec(const char *arg, valcol_t *vc)
{
    char *end = NULL;
    errno = 0;
    unsigned long width = strtoul(arg, &end, 10);
    if ((errno != 0) || (end == arg) || (*end != ':')
            || ((width != 1) && (width != 2) && (width != 4) && (width != 8)))
    {
        return false;
    }
    const char *p = end + 1;
    errno = 0;
    unsigned long dec = strtoul(p, &end, 10);
    if ((errno != 0) || (end == p) || (dec > VKBIN_MAX_DECIMALS)
            || ((*end != ':') && (*end != '\0')))
    {
        return false;
    }
    vc->width = (uint8_t)width;
    vc->decimals = (uint8_t)dec;
    vc->offset = 0;
    vc->na = false;
    // Shifting a 64 bit value by 64 is undefined, so the widest column is a
    // special case.
    vc->maxval = (width == 8) ? UINT64_MAX : ((UINT64_C(1) << (width * 8)) - 1);
    bool hasoffset = false;
    const char *tok = end;
    while (*tok == ':')
    {
        ++tok;
        const char *sep = strchr(tok, ':');
        const size_t toklen = (sep != NULL) ? (size_t)(sep - tok) : strlen(tok);
        if (ascii_ieq(tok, toklen, "NA"))
        {
            if (vc->na)
            {
                return false;
            }
            vc->na = true;
        }
        else
        {
            if (hasoffset || !parse_scaled(tok, toklen, vc->decimals, &vc->offset))
            {
                return false;
            }
            hasoffset = true;
        }
        tok += toklen;
    }
    return true;
}

/**
 * @brief Writes an unsigned value of the given width in little-endian order.
 *
 * Used for the header fields, which do not go through a column buffer.
 *
 * @param f     Destination stream.
 * @param v     Value to write.
 * @param width Number of bytes to write: 1, 2, 4 or 8.
 *
 * @return True on success.
 */
static bool write_uint_le(FILE *f, uint64_t v, uint8_t width)
{
    uint8_t b[8] = {0};
    store_uint_le(b, v, width);
    return (fwrite(b, 1, width, f) == width);
}

/**
 * @brief Creates a spool file for one value column.
 *
 * The file is created beside the output, so that it lands on the same
 * filesystem, and is unlinked as soon as it is opened.
 *
 * @param output Path of the output file.
 * @param idx    Column number, used to build a unique name.
 *
 * @return The open stream, or NULL on failure.
 */
static FILE *open_spool(const char *output, size_t idx)
{
    char path[VKBIN_MAX_PATH];
    const int n = snprintf(path, sizeof(path), "%s.%u.tmp", output, (unsigned)idx);
    if ((n < 0) || (n >= (int)sizeof(path)))
    {
        errno = ENAMETOOLONG;
        return NULL;
    }
    FILE *f = fopen(path, "w+b");
    if (f != NULL)
    {
        (void)remove(path); // the stream keeps the file alive until it is closed
    }
    return f;
}

/**
 * @brief Rounds a file offset up to the given power-of-two alignment.
 *
 * BINSRC1 requires every column to start at a multiple of its item size.
 *
 * @param off   Current offset.
 * @param align Required alignment.
 *
 * @return The aligned offset.
 */
static uint64_t align_up(uint64_t off, uint8_t align)
{
    uint64_t rem = (off % align);
    return (rem == 0) ? off : (off + align - rem);
}

/**
 * @brief Prints the usage text.
 *
 * @param f Destination stream.
 */
static void usage(FILE *f)
{
    (void)fprintf(f,
                  "VariantKey Binary Lookup Table Builder %s\n"
                  "\n"
                  "Builds a BINSRC1 column-oriented binary lookup table, keyed by VariantKey,\n"
                  "from a tab-separated annotation file read on standard input.\n"
                  "\n"
                  "Usage: vkbin -o FILE [-s N] COLSPEC...\n"
                  "\n"
                  "  -o FILE  Output file. Required.\n"
                  "  -s N     Number of leading header lines to skip. Default 1.\n"
                  "  -h       This help.\n"
                  "\n"
                  "The first four input columns must be CHROM, POS, REF and ALT, where POS is\n"
                  "1-based as in VCF. Each COLSPEC describes one further column:\n"
                  "\n"
                  "  WIDTH:DECIMALS[:OFFSET][:NA]\n"
                  "\n"
                  "  WIDTH     Stored size in bytes: 1, 2, 4 or 8.\n"
                  "  DECIMALS  Decimal places to preserve. The value is multiplied by\n"
                  "            10^DECIMALS and rounded to an integer.\n"
                  "  OFFSET    Decimal added before scaling, to shift a signed range into\n"
                  "            the unsigned one. Default 0.\n"
                  "  NA        Store the largest value of WIDTH for a missing field, instead\n"
                  "            of stopping. A real value that reaches that largest value is\n"
                  "            then rejected, so the two cannot be confused at query time.\n"
                  "\n"
                  "OFFSET and NA follow DECIMALS in any order and each may appear once.\n"
                  "\n"
                  "Value fields are decimal numbers, in plain or exponential notation. With\n"
                  "NA a field is missing when it is empty or is '.', 'NA', 'N/A', 'NaN' or\n"
                  "'null', in any case.\n"
                  "\n"
                  "Output columns are the VariantKey as a 64 bit unsigned integer, followed by\n"
                  "one unsigned integer column per COLSPEC, in the given order. Input columns\n"
                  "beyond the last COLSPEC are ignored, so one invocation keeps working when the\n"
                  "source file gains columns.\n"
                  "\n"
                  "The input must already be sorted by VariantKey, as a file sorted by chromosome\n"
                  "and position is. The tool stops on an unsorted input, because an unsorted table\n"
                  "cannot be binary searched and would fail silently at query time.\n"
                  "\n"
                  "Example, AlphaGenome AVI SNV scores. raw_score spans about -1.3 to +4.6, so\n"
                  "an offset of 2.0 shifts it positive; both scores carry 5 decimals:\n"
                  "\n"
                  "  bgzip -dc alphagenome_variant_impact_score_snvs.tsv.gz \\\n"
                  "    | vkbin -o avi.bin 4:5:2.0 4:5:0\n"
                  "\n"
                  "Example, a single 2 byte column of integers with no scaling:\n"
                  "\n"
                  "  vkbin -o out.bin 2:0 < annotations.tsv\n"
                  "\n"
                  "Example, a 4 byte column whose missing fields are stored as 4294967295:\n"
                  "\n"
                  "  vkbin -o out.bin 4:5:NA < annotations.tsv\n",
                  VERSION);
}

/**
 * @brief Reports a fatal input error and its line number.
 *
 * @param line Input line number.
 * @param msg  Description of the problem.
 */
static void input_error(uint64_t line, const char *msg)
{
    (void)fprintf(stderr, "vkbin: line %" PRIu64 ": %s\n", line, msg);
}

/**
 * @brief Reports a fatal output error, adding the system message when one is set.
 *
 * @param path Path of the file being written.
 * @param msg  Description of the problem.
 */
static void output_error(const char *path, const char *msg)
{
    if (errno != 0)
    {
        (void)fprintf(stderr, "vkbin: %s '%s': %s\n", msg, path, strerror(errno));
        return;
    }
    (void)fprintf(stderr, "vkbin: %s '%s'\n", msg, path);
}

/**
 * @brief Tells whether the input holds no further byte.
 *
 * fgets cannot report why it stopped, so a line that fills the buffer is only
 * truncated when something still follows it.
 *
 * @param f Input stream.
 *
 * @return True at the end of the input.
 */
static bool at_end(FILE *f)
{
    const int c = fgetc(f);
    if (c == EOF)
    {
        return true;
    }
    (void)ungetc(c, f);
    return false;
}

/**
 * @brief Discards the remainder of an over-long input line.
 *
 * @param f Input stream.
 *
 * @return True when the line was discarded without a read error.
 */
static bool discard_line(FILE *f)
{
    int c = 0;
    while (((c = fgetc(f)) != EOF) && (c != '\n'))
    {
        // the content is not needed, only the position
    }
    return (ferror(f) == 0);
}

/**
 * @brief Splits a line into at most "max" tab-separated fields.
 *
 * @param line  Start of the line.
 * @param len   Length of the line.
 * @param start Array receiving the start of each field.
 * @param size  Array receiving the length of each field.
 * @param max   Capacity of the arrays.
 *
 * @return The number of fields found, which is "max" when the line has more.
 */
static size_t split_fields(const char *line, size_t len, const char **start, size_t *size, size_t max)
{
    size_t n = 0;
    size_t i = 0;
    size_t begin = 0;
    for (i = 0; ((i <= len) && (n < max)); i++)
    {
        if ((i == len) || (line[i] == '\t'))
        {
            start[n] = (line + begin);
            size[n] = (i - begin);
            ++n;
            begin = (i + 1);
        }
    }
    return n;
}

/**
 * @brief Encodes one input line and spools its columns.
 *
 * @param line     Start of the line.
 * @param len      Length of the line.
 * @param lineno   Input line number, for error messages.
 * @param cols     Value column specifications.
 * @param ncols    Number of value columns.
 * @param vkcol    Writer receiving the VariantKey column.
 * @param prevkey  Previous VariantKey, checked to enforce the sort order.
 * @param fs       Scratch array for the field pointers, owned by the caller.
 * @param fl       Scratch array for the field lengths, owned by the caller.
 *
 * @return True on success.
 */
static bool process_line(const char *line, size_t len, uint64_t lineno,
                         valcol_t *cols, size_t ncols, wbuf_t *vkcol, uint64_t *prevkey,
                         const char **fs, size_t *fl)
{
    const size_t want = (ncols + 4);
    const size_t got = split_fields(line, len, fs, fl, want);
    if (got < want)
    {
        input_error(lineno, "too few columns");
        return false;
    }
    // POS is 1-based in the input and 0-based in a VariantKey. The digits are
    // read here rather than with strtoull, which requires a NUL terminated
    // field.
    uint64_t pos = 0;
    size_t p = 0;
    for (p = 0; p < fl[1]; p++)
    {
        const uint8_t d = (uint8_t)(fs[1][p] - '0');
        if ((d > 9) || (pos > VK_MAX_POS))
        {
            break;
        }
        pos = (pos * 10) + d;
    }
    if ((p != fl[1]) || (fl[1] == 0) || (pos < 1) || ((pos - 1) > VK_MAX_POS))
    {
        input_error(lineno, "POS is not a 1-based integer within the 28 bit range");
        return false;
    }
    const uint64_t vk = variantkey(fs[0], fl[0], (uint32_t)(pos - 1), fs[2], fl[2], fs[3], fl[3]);
    if (vk < *prevkey)
    {
        // A descending key would produce a file that binsearch cannot search.
        input_error(lineno, "input is not sorted by VariantKey");
        return false;
    }
    *prevkey = vk;
    if (!wbuf_put(vkcol, vk, 8))
    {
        (void)fprintf(stderr, "vkbin: cannot write the VariantKey column: %s\n", strerror(errno));
        return false;
    }
    size_t c = 0;
    for (c = 0; c < ncols; c++)
    {
        uint64_t stored = 0;
        if (cols[c].na && is_missing(fs[c + 4], fl[c + 4]))
        {
            stored = cols[c].maxval;
        }
        else
        {
            int64_t v = 0;
            if (!parse_scaled(fs[c + 4], fl[c + 4], cols[c].decimals, &v))
            {
                input_error(lineno, "value column is not a decimal number");
                return false;
            }
            if (((v > 0) && (cols[c].offset > (INT64_MAX - v)))
                    || ((v < 0) && (cols[c].offset < (INT64_MIN - v))))
            {
                input_error(lineno, "value column overflows after the offset is applied");
                return false;
            }
            v += cols[c].offset;
            if ((v < 0) || ((uint64_t)v > cols[c].maxval))
            {
                input_error(lineno, "value column does not fit in the declared width");
                return false;
            }
            stored = (uint64_t)v;
            if (cols[c].na && (stored == cols[c].maxval))
            {
                // Storing it would make the row indistinguishable from a
                // missing one at query time.
                input_error(lineno, "value column collides with the missing value marker");
                return false;
            }
        }
        if (!wbuf_put(&cols[c].spool, stored, cols[c].width))
        {
            (void)fprintf(stderr, "vkbin: cannot write a value column: %s\n", strerror(errno));
            return false;
        }
    }
    return true;
}

/**
 * @brief Writes the BINSRC1 header and appends the spooled value columns.
 *
 * The header cannot be written before the input is consumed because it carries
 * the number of rows, so the VariantKey column is written after a placeholder
 * of the same size and the header is rewritten at the end.
 *
 * @param out    Output stream, positioned after the VariantKey column.
 * @param cols   Value column specifications.
 * @param ncols  Number of value columns.
 * @param nrows  Number of rows written.
 * @param hdrlen Size of the header placeholder.
 *
 * @return True on success.
 */
static bool finalize(FILE *out, valcol_t *cols, size_t ncols, uint64_t nrows, uint64_t hdrlen)
{
    static uint8_t buf[VKBIN_BUFSIZE];
    const size_t total = (ncols + 1);
    uint64_t index[MAXCOLS];
    index[0] = hdrlen;
    uint64_t off = (hdrlen + (nrows * 8));
    size_t c = 0;
    errno = 0; // a failure below is reported with the system message it sets
    for (c = 0; c < ncols; c++)
    {
        const uint64_t aligned = align_up(off, cols[c].width);
        while (off < aligned)
        {
            if (fputc(0, out) == EOF)
            {
                return false;
            }
            ++off;
        }
        index[c + 1] = off;
        off += (nrows * cols[c].width);
        // Copy the spool back through a fixed buffer: the column can be far
        // larger than memory.
        if ((fflush(cols[c].spool.f) != 0) || (fseek(cols[c].spool.f, 0, SEEK_SET) != 0))
        {
            return false;
        }
        size_t n = 0;
        while ((n = fread(buf, 1, sizeof(buf), cols[c].spool.f)) > 0)
        {
            if (fwrite(buf, 1, n, out) != n)
            {
                return false;
            }
        }
        // fread stops on both end of file and error, which are told apart here.
        if (ferror(cols[c].spool.f) != 0)
        {
            return false;
        }
    }
    if (fseek(out, 0, SEEK_SET) != 0)
    {
        return false;
    }
    if (fwrite("BINSRC1\0", 1, 8, out) != 8)
    {
        return false;
    }
    // The column count is followed by one byte per column giving its item size,
    // then by padding so that the row count starts on an 8 byte boundary.
    uint8_t meta[MAXCOLS + 8];
    memset(meta, 0, sizeof(meta));
    meta[0] = (uint8_t)total;
    meta[1] = 8; // the VariantKey column
    for (c = 0; c < ncols; c++)
    {
        meta[c + 2] = cols[c].width;
    }
    const uint64_t metalen = (hdrlen - 8 - ((total + 1) * 8));
    if (fwrite(meta, 1, (size_t)metalen, out) != (size_t)metalen)
    {
        return false;
    }
    if (!write_uint_le(out, nrows, 8))
    {
        return false;
    }
    for (c = 0; c < total; c++)
    {
        if (!write_uint_le(out, index[c], 8))
        {
            return false;
        }
    }
    return true;
}

int main(int argc, char *argv[])
{
    const char *output = NULL;
    uint64_t skip = 1;
    // getopt is POSIX and is not declared under a strict C17 compilation, so
    // the options are parsed by hand.
    int argi = 1;
    while ((argi < argc) && (argv[argi][0] == '-') && (argv[argi][1] != '\0'))
    {
        const char *opt = argv[argi];
        if (strcmp(opt, "-h") == 0)
        {
            usage(stdout);
            return 0;
        }
        if ((strcmp(opt, "-o") != 0) && (strcmp(opt, "-s") != 0))
        {
            (void)fprintf(stderr, "vkbin: unknown option '%s'\n\n", opt);
            usage(stderr);
            return 1;
        }
        if ((argi + 1) >= argc)
        {
            (void)fprintf(stderr, "vkbin: option '%s' requires an argument\n\n", opt);
            usage(stderr);
            return 1;
        }
        const char *val = argv[argi + 1];
        if (strcmp(opt, "-o") == 0)
        {
            output = val;
        }
        else
        {
            char *end = NULL;
            errno = 0;
            unsigned long long v = strtoull(val, &end, 10);
            // strtoull also accepts a sign and leading spaces, so the first
            // character is checked.
            if ((errno != 0) || (end == val) || (*end != '\0') || (val[0] < '0'))
            {
                (void)fprintf(stderr, "vkbin: -s requires a non-negative integer, got '%s'\n", val);
                return 1;
            }
            skip = (uint64_t)v;
        }
        argi += 2;
    }
    if (output == NULL)
    {
        (void)fprintf(stderr, "vkbin: -o is required\n\n");
        usage(stderr);
        return 1;
    }
    const size_t ncols = (size_t)(argc - argi);
    if ((ncols == 0) || (ncols > VKBIN_MAX_VALCOLS))
    {
        (void)fprintf(stderr, "vkbin: between 1 and %d column specifications are required\n\n",
                      VKBIN_MAX_VALCOLS);
        usage(stderr);
        return 1;
    }
    valcol_t cols[VKBIN_MAX_VALCOLS];
    memset(cols, 0, sizeof(cols));
    size_t c = 0;
    for (c = 0; c < ncols; c++)
    {
        if (!parse_colspec(argv[argi + c], &cols[c]))
        {
            (void)fprintf(stderr, "vkbin: malformed column specification '%s'\n\n",
                          argv[argi + c]);
            usage(stderr);
            return 1;
        }
    }
    // The output is built under a temporary name carrying the process id, and
    // renamed once it is complete, so that a failed run leaves an existing file
    // untouched and two concurrent runs do not share the same name.
    char tmppath[VKBIN_MAX_PATH];
    const int pathlen = snprintf(tmppath, sizeof(tmppath), "%s.%ld.vkbin.tmp",
                                 output, (long)getpid());
    if ((pathlen < 0) || (pathlen >= (int)sizeof(tmppath)))
    {
        (void)fprintf(stderr, "vkbin: the output path is too long\n");
        return 1;
    }

    int rv = 1;
    const size_t total = (ncols + 1);
    uint64_t nrows = 0;
    // Field scratch, initialised once instead of once per input line.
    const char *fs[VKBIN_MAX_VALCOLS + 4] = {NULL};
    size_t fl[VKBIN_MAX_VALCOLS + 4] = {0};
    static char inbuf[VKBIN_BUFSIZE];
    (void)setvbuf(stdin, inbuf, _IOFBF, sizeof(inbuf));
    FILE *out = fopen(tmppath, "w+b");
    if (out == NULL)
    {
        (void)fprintf(stderr, "vkbin: cannot open '%s': %s\n", tmppath, strerror(errno));
        return 1;
    }
    static char outbuf[VKBIN_BUFSIZE];
    (void)setvbuf(out, outbuf, _IOFBF, sizeof(outbuf));
    // One allocation backs the write buffer of the VariantKey column and of
    // every value column.
    wbuf_t vkcol = { out, NULL, 0 };
    uint8_t *wbufmem = (uint8_t *)malloc(total * (size_t)VKBIN_WBUFSIZE);
    if (wbufmem == NULL)
    {
        (void)fprintf(stderr, "vkbin: cannot allocate the write buffers\n");
        goto cleanup;
    }
    vkcol.buf = wbufmem;
    for (c = 0; c < ncols; c++)
    {
        cols[c].spool.buf = (wbufmem + ((c + 1) * (size_t)VKBIN_WBUFSIZE));
        cols[c].spool.f = open_spool(tmppath, c);
        if (cols[c].spool.f == NULL)
        {
            (void)fprintf(stderr, "vkbin: cannot create a spool file: %s\n", strerror(errno));
            goto cleanup;
        }
    }

    // Reserve the header, which can only be written once the row count is known.
    const uint64_t hdrlen = (9 + total + ((8 - ((total + 1) & 7)) & 7) + ((total + 1) * 8));
    uint64_t i = 0;
    for (i = 0; i < hdrlen; i++)
    {
        if (fputc(0, out) == EOF)
        {
            output_error(tmppath, "cannot write to");
            goto cleanup;
        }
    }

    char line[VKBIN_MAX_LINE];
    uint64_t lineno = 0;
    uint64_t prevkey = 0;
    while (fgets(line, sizeof(line), stdin) != NULL)
    {
        ++lineno;
        size_t len = strlen(line);
        // fgets stops at the buffer limit without telling the caller, so a
        // truncated line is one that filled the buffer, has no terminator and
        // is not the last of the input.
        if ((len == (sizeof(line) - 1)) && (line[len - 1] != '\n') && !at_end(stdin))
        {
            if (lineno > skip)
            {
                input_error(lineno, "line is too long");
                goto cleanup;
            }
            // A skipped line is discarded whole, so its length does not matter.
            if (!discard_line(stdin))
            {
                (void)fprintf(stderr, "vkbin: cannot read the input: %s\n", strerror(errno));
                goto cleanup;
            }
            continue;
        }
        while ((len > 0) && ((line[len - 1] == '\n') || (line[len - 1] == '\r')))
        {
            --len;
        }
        if (lineno <= skip)
        {
            continue;
        }
        if (len == 0)
        {
            continue; // a trailing blank line is not an error
        }
        if (!process_line(line, len, lineno, cols, ncols, &vkcol, &prevkey, fs, fl))
        {
            goto cleanup;
        }
        ++nrows;
    }
    if (ferror(stdin))
    {
        (void)fprintf(stderr, "vkbin: cannot read the input: %s\n", strerror(errno));
        goto cleanup;
    }
    if (nrows == 0)
    {
        (void)fprintf(stderr, "vkbin: the input has no data row\n");
        goto cleanup;
    }
    errno = 0;
    if (!wbuf_flush(&vkcol))
    {
        output_error(tmppath, "cannot write to");
        goto cleanup;
    }
    for (c = 0; c < ncols; c++)
    {
        if (!wbuf_flush(&cols[c].spool))
        {
            (void)fprintf(stderr, "vkbin: cannot write a value column: %s\n", strerror(errno));
            goto cleanup;
        }
    }
    if (!finalize(out, cols, ncols, nrows, hdrlen))
    {
        output_error(tmppath, "cannot write");
        goto cleanup;
    }
    rv = 0;

cleanup:
    for (c = 0; c < ncols; c++)
    {
        if (cols[c].spool.f != NULL)
        {
            (void)fclose(cols[c].spool.f);
        }
    }
    free(wbufmem);
    errno = 0;
    if (fclose(out) != 0)
    {
        output_error(tmppath, "cannot close");
        rv = 1;
    }
    if (rv == 0)
    {
        errno = 0;
        if (rename(tmppath, output) != 0)
        {
            output_error(output, "cannot rename the output to");
            rv = 1;
        }
    }
    if (rv != 0)
    {
        (void)remove(tmppath); // never leave a truncated file behind
        return rv;
    }
    (void)fprintf(stderr, "vkbin: %" PRIu64 " rows, %zu columns, written to '%s'\n",
                  nrows, total, output);
    return rv;
}
