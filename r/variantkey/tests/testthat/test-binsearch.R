context("variantkey")
library(variantkey)

# test-binsearch.R
# @category   Libraries
# @author     Nicola Asuni <info@tecnick.com>
# @link       https://github.com/tecnickcom/variantkey
# @license    MIT (see LICENSE file)

test_that("MmapGenorefFile", {
    gref <<- MmapGenorefFile("../../../../c/test/data/genoref.bin")
    expect_that(gref$SIZE, equals(598))
})

test_that("MunmapBinfile", {
    err <- MunmapBinfile(gref$MF)
    expect_that(err, equals(0))
})
