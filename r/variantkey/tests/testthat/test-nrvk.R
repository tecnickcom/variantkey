context("variantkey")
library(variantkey)

# test-nrvk.R
# @category   Libraries
# @author     Nicola Asuni <info@tecnick.com>
# @link       https://github.com/tecnickcom/variantkey
# @license    MIT [LICENSE](https://raw.githubusercontent.com/tecnickcom/variantkey/main/LICENSE)

x <- rbind(
    list("0800c35093ace339",  "1", 100001, 0x00000004, 0x01, 0x01, "00000000100186a1", "00000000100186a2", "N", "A"),
    list("1000c3517f91cdb1",  "2", 100002, 0x0000000e, 0x0b, 0x01, "00000000200186a2", "00000000200186ad", "AAGAAAGAAAG", "A"),
    list("1800c351f61f65d3",  "3", 100003, 0x0000000e, 0x01, 0x0b, "00000000300186a3", "00000000300186a4", "A", "AAGAAAGAAAG"),
    list("2000c3521f1c15ab",  "4", 100004, 0x0000000e, 0x08, 0x04, "00000000400186a4", "00000000400186ac", "ACGTACGT", "ACGT"),
    list("2800c352d8f2d5b5",  "5", 100005, 0x0000000e, 0x04, 0x08, "00000000500186a5", "00000000500186a9", "ACGT", "ACGTACGT"),
    list("5000c3553bbf9c19", "10", 100010, 0x00000012, 0x08, 0x08, "00000000a00186aa", "00000000a00186b2", "ACGTACGT", "CGTACGTA"),
    list("b000c35b64690b25", "22", 100022, 0x0000000b, 0x08, 0x01, "00000001600186b6", "00000001600186be", "ACGTACGT", "N"),
    list("b800c35bbcece603",  "X", 100023, 0x0000000e, 0x0a, 0x02, "00000001700186b7", "00000001700186c1", "AAAAAAAAGG", "AG"),
    list("c000c35c63741ee7",  "Y", 100024, 0x0000000e, 0x02, 0x0a, "00000001800186b8", "00000001800186ba", "AG", "AAAAAAAAGG"),
    list("c800c35c96c18499", "MT", 100025, 0x00000012, 0x04, 0x0c, "00000001900186b9", "00000001900186bd", "ACGT", "AAACCCGGGTTT")
)
colnames(x) <- list("vk", "chrom", "pos", "ralen", "sizeref", "sizealt", "csp", "cep", "ref", "alt")

InitVariantKey(nrvk_file = "../../../../c/test/data/nrvk.10.bin")

test_that("FindRefAltByVariantKey", {
    res <- FindRefAltByVariantKey(vk = as.hex64(unlist(x[,"vk"])))
    expect_that(res$REF, equals(unlist(x[,"ref"])))
    expect_that(res$ALT, equals(unlist(x[,"alt"])))
})

test_that("ReverseVariantKey", {
    res <- ReverseVariantKey(vk = as.hex64(unlist(x[,"vk"])))
    expect_that(res$CHROM, equals(unlist(x[,"chrom"])))
    expect_that(res$POS, equals(unlist(x[,"pos"])))
    expect_that(res$REF, equals(unlist(x[,"ref"])))
    expect_that(res$ALT, equals(unlist(x[,"alt"])))
})

test_that("GetVariantKeyRefLength", {
    res <- GetVariantKeyRefLength(vk = as.hex64(unlist(x[,"vk"])))
    expect_that(res, equals(unlist(x[,"sizeref"])))
})

test_that("GetVariantKeyRefLengthReversible", {
    res <- GetVariantKeyRefLength(as.hex64("1800925199160000"))
    expect_that(res, equals(3))
})

test_that("GetVariantKeyRefLengthNotFound", {
    res <- GetVariantKeyRefLength(as.hex64("ffffffffffffffff"))
    expect_that(res, equals(0))
})

test_that("GetVariantKeyEndPos", {
    res <- GetVariantKeyEndPos(vk = as.hex64(unlist(x[,"vk"])))
    expect_that(res, equals(unlist(x[,"pos"]) + unlist(x[,"sizeref"])))
})

test_that("GetVariantKeyChromStartPos", {
    res <- GetVariantKeyChromStartPos(vk = as.hex64(unlist(x[,"vk"])))
    expect_that(res, equals(as.uint64(as.hex64(unlist(x[,"csp"])))))
})

test_that("GetVariantKeyChromEndPos", {
    res <- GetVariantKeyChromEndPos(vk = as.hex64(unlist(x[,"vk"])))
    expect_that(res, equals(as.uint64(as.hex64(unlist(x[,"cep"])))))
})

test_that("VknrBinToTsv", {
    size <- VknrBinToTsv("nrvk.test")
    expect_that(size, equals(305))
})

CloseVariantKey()
