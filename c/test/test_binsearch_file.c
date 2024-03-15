// binsearch
//
// test_binsearch_file.c
//
// @category   Test
// @author     Nicola Asuni <info@tecnick.com>
// @link       https://github.com/tecnickcom/binsearch
// @license    MIT [LICENSE](https://raw.githubusercontent.com/tecnickcom/variantkey/main/LICENSE)

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/mman.h>
#include "../src/variantkey/binsearch.h"

int test_mmap_binfile_error(const char* file)
{
    mmfile_t mf = {0};
    mmap_binfile(file, &mf);
    if (mf.src != MAP_FAILED)
    {
        (void) fprintf(stderr, "An mmap error was expected\n");
        return 1;
    }
    return 0;
}

int test_munmap_binfile_error()
{
    mmfile_t mf = {0};
    int e = munmap_binfile(mf);
    if (e == 0)
    {
        (void) fprintf(stderr, "An mummap error was expected\n");
        return 1;
    }
    return 0;
}

int test_map_file_arrow()
{
    int errors = 0;
    char *file = "test_data_arrow.bin"; // file containing test data
    mmfile_t mf = {0};
    mf.ncols = 2;
    mf.ctbytes[0] = 4;
    mf.ctbytes[1] = 8;
    mmap_binfile(file, &mf);
    if (mf.fd < 0)
    {
        (void) fprintf(stderr, "%s can't open %s for reading\n", __func__, file);
        return 1;
    }
    if (mf.size == 0)
    {
        (void) fprintf(stderr, "%s fstat error! [%s]\n", __func__, strerror(errno));
        return 1;
    }
    if (mf.src == MAP_FAILED)
    {
        (void) fprintf(stderr, "%s mmap error! [%s]\n", __func__, strerror(errno));
        return 1;
    }
    if (mf.size != 730)
    {
        (void) fprintf(stderr, "%s mf.size : Expecting 730 bytes, got instead: %" PRIu64 "\n", __func__, mf.size);
        errors++;
    }
    if (mf.doffset != 376)
    {
        (void) fprintf(stderr, "%s mf.doffset : Expecting 376 bytes, got instead: %" PRIu64 "\n", __func__, mf.doffset);
        errors++;
    }
    if (mf.dlength != 136)
    {
        (void) fprintf(stderr, "%s mf.dlength : Expecting 136 bytes, got instead: %" PRIu64 "\n", __func__, mf.dlength);
        errors++;
    }
    if (mf.nrows != 11)
    {
        (void) fprintf(stderr, "%s mf.nrows : Expecting 11 items, got instead: %" PRIu64 "\n", __func__, mf.nrows);
        errors++;
    }
    if (mf.ncols != 2)
    {
        (void) fprintf(stderr, "%s mf.ncols : Expecting 2 items, got instead: %" PRIu8 "\n", __func__, mf.ncols);
        errors++;
    }
    if (mf.index[0] != 376)
    {
        (void) fprintf(stderr, "%s mf.index[0] : Expecting 376 bytes, got instead: %" PRIu64 "\n", __func__, mf.index[0]);
        errors++;
    }
    if (mf.index[1] != 424)
    {
        (void) fprintf(stderr, "%s mf.index[1] : Expecting 424 bytes, got instead: %" PRIu64 "\n", __func__, mf.index[1]);
        errors++;
    }
    int e = munmap_binfile(mf);
    if (e != 0)
    {
        (void) fprintf(stderr, "%s Got %d error while unmapping the file\n", __func__, e);
        errors++;
    }
    return errors;
}

int test_map_file_feather()
{
    int errors = 0;
    char *file = "test_data_feather.bin"; // file containing test data
    mmfile_t mf = {0};
    mf.ncols = 2;
    mf.ctbytes[0] = 4;
    mf.ctbytes[1] = 8;
    mmap_binfile(file, &mf);
    if (mf.fd < 0)
    {
        (void) fprintf(stderr, "%s can't open %s for reading\n", __func__, file);
        return 1;
    }
    if (mf.size == 0)
    {
        (void) fprintf(stderr, "%s fstat error! [%s]\n", __func__, strerror(errno));
        return 1;
    }
    if (mf.src == MAP_FAILED)
    {
        (void) fprintf(stderr, "%s mmap error! [%s]\n", __func__, strerror(errno));
        return 1;
    }
    if (mf.size != 384)
    {
        (void) fprintf(stderr, "%s mf.size : Expecting 384 bytes, got instead: %" PRIu64 "\n", __func__, mf.size);
        errors++;
    }
    if (mf.doffset != 8)
    {
        (void) fprintf(stderr, "%s mf.doffset : Expecting 8 bytes, got instead: %" PRIu64 "\n", __func__, mf.doffset);
        errors++;
    }
    if (mf.dlength != 136)
    {
        (void) fprintf(stderr, "%s mf.dlength : Expecting 136 bytes, got instead: %" PRIu64 "\n", __func__, mf.dlength);
        errors++;
    }
    if (mf.nrows != 11)
    {
        (void) fprintf(stderr, "%s mf.nrows : Expecting 11 items, got instead: %" PRIu64 "\n", __func__, mf.nrows);
        errors++;
    }
    if (mf.ncols != 2)
    {
        (void) fprintf(stderr, "%s mf.ncols : Expecting 2 items, got instead: %" PRIu8 "\n", __func__, mf.ncols);
        errors++;
    }
    if (mf.index[0] != 8)
    {
        (void) fprintf(stderr, "%s mf.index[0] : Expecting 8 bytes, got instead: %" PRIu64 "\n", __func__, mf.index[0]);
        errors++;
    }
    if (mf.index[1] != 56)
    {
        (void) fprintf(stderr, "%s mf.index[1] : Expecting 56 bytes, got instead: %" PRIu64 "\n", __func__, mf.index[1]);
        errors++;
    }
    int e = munmap_binfile(mf);
    if (e != 0)
    {
        (void) fprintf(stderr, "%s Got %d error while unmapping the file\n", __func__, e);
        errors++;
    }
    return errors;
}

int test_map_file_binsrc()
{
    int errors = 0;
    char *file = "test_data_binsrc.bin"; // file containing test data
    mmfile_t mf = {0};
    mmap_binfile(file, &mf);
    if (mf.fd < 0)
    {
        (void) fprintf(stderr, "%s can't open %s for reading\n", __func__, file);
        return 1;
    }
    if (mf.size == 0)
    {
        (void) fprintf(stderr, "%s fstat error! [%s]\n", __func__, strerror(errno));
        return 1;
    }
    if (mf.src == MAP_FAILED)
    {
        (void) fprintf(stderr, "%s mmap error! [%s]\n", __func__, strerror(errno));
        return 1;
    }
    if (mf.size != 176)
    {
        (void) fprintf(stderr, "%s mf.size : Expecting 176 bytes, got instead: %" PRIu64 "\n", __func__, mf.size);
        errors++;
    }
    if (mf.doffset != 40)
    {
        (void) fprintf(stderr, "%s mf.doffset : Expecting 40 bytes, got instead: %" PRIu64 "\n", __func__, mf.doffset);
        errors++;
    }
    if (mf.dlength != 136)
    {
        (void) fprintf(stderr, "%s mf.dlength : Expecting 136 bytes, got instead: %" PRIu64 "\n", __func__, mf.dlength);
        errors++;
    }
    if (mf.nrows != 11)
    {
        (void) fprintf(stderr, "%s mf.nrows : Expecting 11 items, got instead: %" PRIu64 "\n", __func__, mf.nrows);
        errors++;
    }
    if (mf.ncols != 2)
    {
        (void) fprintf(stderr, "%s mf.ncols : Expecting 2 items, got instead: %" PRIu8 "\n", __func__, mf.ncols);
        errors++;
    }
    if (mf.index[0] != 40)
    {
        (void) fprintf(stderr, "%s mf.index[0] : Expecting 40 bytes, got instead: %" PRIu64 "\n", __func__, mf.index[0]);
        errors++;
    }
    if (mf.index[1] != 88)
    {
        (void) fprintf(stderr, "%s mf.index[1] : Expecting 88 bytes, got instead: %" PRIu64 "\n", __func__, mf.index[1]);
        errors++;
    }
    int e = munmap_binfile(mf);
    if (e != 0)
    {
        (void) fprintf(stderr, "%s Got %d error while unmapping the file\n", __func__, e);
        errors++;
    }
    return errors;
}

int test_map_file_col()
{
    int errors = 0;
    char *file = "test_data_col.bin"; // file containing test data
    mmfile_t mf = {0};
    mmap_binfile(file, &mf);
    if (mf.fd < 0)
    {
        (void) fprintf(stderr, "%s can't open %s for reading\n", __func__, file);
        return 1;
    }
    if (mf.size == 0)
    {
        (void) fprintf(stderr, "%s fstat error! [%s]\n", __func__, strerror(errno));
        return 1;
    }
    if (mf.src == MAP_FAILED)
    {
        (void) fprintf(stderr, "%s mmap error! [%s]\n", __func__, strerror(errno));
        return 1;
    }
    if (mf.size != 3776)
    {
        (void) fprintf(stderr, "%s mf.size : Expecting 3776 bytes, got instead: %" PRIu64 "\n", __func__, mf.size);
        errors++;
    }
    if (mf.doffset != 0)
    {
        (void) fprintf(stderr, "%s mf.doffset : Expecting 0 bytes, got instead: %" PRIu64 "\n", __func__, mf.doffset);
        errors++;
    }
    if (mf.dlength != 3776)
    {
        (void) fprintf(stderr, "%s mf.dlength : Expecting 3776 bytes, got instead: %" PRIu64 "\n", __func__, mf.dlength);
        errors++;
    }
    if (mf.nrows != 0)
    {
        (void) fprintf(stderr, "%s mf.nrows : Expecting 0 items, got instead: %" PRIu64 "\n", __func__, mf.nrows);
        errors++;
    }
    if (mf.ncols != 0)
    {
        (void) fprintf(stderr, "%s mf.ncols : Expecting 0 items, got instead: %" PRIu8 "\n", __func__, mf.ncols);
        errors++;
    }
    int e = munmap_binfile(mf);
    if (e != 0)
    {
        (void) fprintf(stderr, "%s Got %d error while unmapping the file\n", __func__, e);
        errors++;
    }
    return errors;
}

int main()
{
    int errors = 0;

    errors += test_mmap_binfile_error("ERROR");
    errors += test_mmap_binfile_error("/dev/null");
    errors += test_munmap_binfile_error();
    errors += test_map_file_arrow();
    errors += test_map_file_feather();
    errors += test_map_file_binsrc();
    errors += test_map_file_col();

    return errors;
}
