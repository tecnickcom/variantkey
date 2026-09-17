# R VariantKey Wrapper
#
# variantkey.R
#
# @category Libraries
# @author   Nicola Asuni <info@tecnick.com>
# @link     https://github.com/tecnickcom/variantkey
# @license  MIT [LICENSE](https://raw.githubusercontent.com/tecnickcom/variantkey/main/LICENSE)

# Define an environment for this package
vk.env <- new.env(parent = emptyenv())

# Internal objects for memory-mapped files
vk.env$genoref_ <- list(MF=NULL, SIZE=0)
vk.env$nrvk_ <- list(MF=NULL, MC=NULL, NROWS=0)
vk.env$rsvk_ <- list(MF=NULL, MC=NULL, NROWS=0)
vk.env$vkrs_ <- list(MF=NULL, MC=NULL, NROWS=0)

# Common error message
vk.env$ERR_INPUT_LENGTH <- "Error: input vectors must have the same length."

# Convert numeric vectors in the unsigned 32-bit domain to R integers preserving bit patterns.
vk.env$as_uint32_bits <- function(x, argname) {
  if (is.integer(x)) {
    return(x)
  }
  if (!is.numeric(x)) {
    stop(paste0("Error: ", argname, " must be numeric."))
  }
  if (any(!is.finite(x))) {
    stop(paste0("Error: ", argname, " must contain finite values only."))
  }
  if (any(x < 0) || any(x > 4294967295)) {
    stop(paste0("Error: ", argname, " must be in range [0, 4294967295]."))
  }
  if (any(x != floor(x))) {
    stop(paste0("Error: ", argname, " must contain integer values only."))
  }
  y <- ifelse(x > 2147483647, x - 4294967296, x)
  return(as.integer(y))
}

#' Loads the VariantKey support files.
#' This must be the first function called.
#' @param genoref_file Name and path of the binary file containing the genome reference (fasta.bin). This file can be generated from a FASTA file using the resources/tools/fastabin.sh script.
#' @param nrvk_file  Name and path of the binary file containing the non-reversible-VariantKey mapping (nrvk.bin). This file can be generated from a normalized VCF file using the resources/tools/nrvk.sh script.
#' @param rsvk_file  Name and path of the binary file containing the rsID to VariantKey mapping (rsvk.bin). This file can be generated using the resources/tools/rsvk.sh script.
#' @param vkrs_file  Name and path of the binary file containing the VariantKey to rsID mapping (vkrs.bin). This file can be generated using the resources/tools/vkrs.sh script.
#' @export
InitVariantKey <- function(genoref_file = "", nrvk_file = "", rsvk_file = "", vkrs_file = "") {
  errors <- character()
  if (genoref_file != "") {
    # Load the reference genome binary file.
    genoref <- MmapGenorefFile(genoref_file)
    if (genoref$SIZE <= 0) {
      stop(paste("Unable to load the GENOREF file: ", genoref_file, sep = ""))
    }
    vk.env$genoref_ <- genoref
  }
  if (nrvk_file != "") {
    # Load the lookup table for non-reversible variantkeys.
    nrvk <- MmapNRVKFile(nrvk_file)
    if (nrvk$NROWS <= 0) {
      stop(paste("Unable to load the NRVK file: ", nrvk_file, sep = ""))
    }
    vk.env$nrvk_ <- nrvk
  }
  if (rsvk_file != "") {
    # Load the lookup table for rsID to VariantKey.
    rsvk <- MmapRSVKFile(rsvk_file, as.integer(c(4, 8)))
    if (rsvk$NROWS <= 0) {
      stop(paste("Unable to load the RSVK file: ", rsvk_file, sep = ""))
    }
    vk.env$rsvk_ <- rsvk
  }
  if (vkrs_file != "") {
    # Load the lookup table for VariantKey to rsID
    vkrs <- MmapVKRSFile(vkrs_file, as.integer(c(8, 4)))
    if (vkrs$NROWS <= 0) {
      stop(paste("Unable to load the VKRS file: ", vkrs_file, sep = ""))
    }
    vk.env$vkrs_ <- vkrs
  }
}

#' Unmaps the memory mapped files.
#' This must be the last function called.
#' @export
CloseVariantKey <- function() {
  # The internal objects are reset to their initial value after unmapping, so
  # calling this twice is a no-op and any later lookup finds no table instead of
  # reading through the pointers of the released mapping.
  if (vk.env$genoref_$SIZE > 0) {
    MunmapBinfile(vk.env$genoref_$MF)
    vk.env$genoref_ <- list(MF=NULL, SIZE=0)
  }
  if (vk.env$nrvk_$NROWS > 0) {
    MunmapBinfile(vk.env$nrvk_$MF)
    vk.env$nrvk_ <- list(MF=NULL, MC=NULL, NROWS=0)
  }
  if (vk.env$rsvk_$NROWS > 0) {
    MunmapBinfile(vk.env$rsvk_$MF)
    vk.env$rsvk_ <- list(MF=NULL, MC=NULL, NROWS=0)
  }
  if (vk.env$vkrs_$NROWS > 0) {
    MunmapBinfile(vk.env$vkrs_$MF)
    vk.env$vkrs_ <- list(MF=NULL, MC=NULL, NROWS=0)
  }
  invisible(NULL)
}

#' Encodes a chromosome identifier into a numerical code.
#' @param chrom Chromosome. An identifier from the reference genome, no white-space or leading zeros permitted.
#' @useDynLib variantkey R_encode_chrom
#' @export
EncodeChrom <- function(chrom) {
  ret <- integer(length(chrom))
  return(.Call("R_encode_chrom", as.character(chrom), ret))
}

#' Decodes a chromosome numerical code into its string representation.
#' @param code Chromosome numerical code.
#' @useDynLib variantkey R_decode_chrom
#' @export
DecodeChrom <- function(code) {
  ret <- character(length(code))
  return(.Call("R_decode_chrom", as.integer(code), ret))
}

#' Encodes a REF+ALT pair into a 31 bit code.
#' @param ref Reference allele. String containing a sequence of nucleotide letters.
#' @param alt Alternate non-reference allele string.
#' @useDynLib variantkey R_encode_refalt
#' @export
EncodeRefAlt <- function(ref, alt) {
  n <- length(ref)
  if (n != length(alt)) {
    stop(vk.env$ERR_INPUT_LENGTH)
  }
  ret <- integer(n)
  return(.Call("R_encode_refalt", as.character(ref), as.character(alt), ret))
}

#' Decodes a 32 bit REF+ALT code if it was produced by the reversible encoding (11 or less bases in total, containing only A, C, G and T letters).
#' @param code REF+ALT code
#' @useDynLib variantkey R_decode_refalt
#' @export
DecodeRefAlt <- function(code) {
  ref <- character(length(code))
  alt <- character(length(code))
  return(.Call("R_decode_refalt", as.integer(code), ref, alt))
}

#' Assembles a VariantKey from the pre-encoded CHROM, POS and REF+ALT.
#' @param chrom   Encoded Chromosome (see EncodeChrom)
#' @param pos   Position. The reference position, with the first base having position 0.
#' @param refalt  Encoded Reference + Alternate (see EncodeRefAlt)
#' @useDynLib   variantkey R_encode_variantkey
#' @export
EncodeVariantKey <- function(chrom, pos, refalt) {
  n <- length(chrom)
  if ((n != length(pos)) || (n != length(refalt))) {
    stop(vk.env$ERR_INPUT_LENGTH)
  }
  ret <- uint64(n)
  return(.Call("R_encode_variantkey", as.integer(chrom), vk.env$as_uint32_bits(pos, "pos"), as.integer(refalt), ret))
}

#' Extracts the CHROM code from a VariantKey.
#' @param vk VariantKey code.
#' @useDynLib   variantkey R_extract_variantkey_chrom
#' @export
ExtractVariantKeyChrom <- function(vk) {
  ret <- integer(length(vk))
  return(.Call("R_extract_variantkey_chrom", as.uint64(vk), ret))
}

#' Extracts the POS value from a VariantKey.
#' @param vk VariantKey code.
#' @useDynLib   variantkey R_extract_variantkey_pos
#' @export
ExtractVariantKeyPos <- function(vk) {
  ret <- integer(length(vk))
  return(.Call("R_extract_variantkey_pos", as.uint64(vk), ret))
}

#' Extracts the REF+ALT code from a VariantKey.
#' @param vk VariantKey code.
#' @useDynLib   variantkey R_extract_variantkey_refalt
#' @export
ExtractVariantKeyRefAlt <- function(vk) {
  ret <- integer(length(vk))
  return(.Call("R_extract_variantkey_refalt", as.uint64(vk), ret))
}

#' Splits a VariantKey into its CHROM, POS and REF+ALT components.
#' @param code VariantKey code.
#' @param vk   Decoded variantkey structure.
#' @useDynLib   variantkey R_decode_variantkey
#' @export
DecodeVariantKey <- function(vk) {
  n <- length(vk)
  chrom <- integer(n)
  pos <- integer(n)
  refalt <- integer(n)
  return(.Call("R_decode_variantkey", as.uint64(vk), chrom, pos, refalt))
}

#' Returns a VariantKey for the given CHROM, POS (0-based), REF and ALT.
#' The variant should be already normalized (see NormalizeVariant or use NormalizedVariantkey).
#' @param chrom Chromosome. An identifier from the reference genome, no white-space or leading zeros permitted.
#' @param pos   Position. The reference position, with the first base having position 0.
#' @param ref   Reference allele. String containing a sequence of nucleotide letters.
#' @param alt   Alternate non-reference allele string.
#' @useDynLib   variantkey R_variantkey
#' @export
VariantKey <- function(chrom, pos, ref, alt) {
  n <- length(pos)
  if ((n != length(chrom)) || (n != length(ref)) || (n != length(alt))) {
    stop(vk.env$ERR_INPUT_LENGTH)
  }
  ret <- uint64(n)
  return(.Call("R_variantkey", as.character(chrom), vk.env$as_uint32_bits(pos, "pos"), as.character(ref), as.character(alt), ret))
}

#' Returns the minimum and maximum VariantKey of a CHROM and POS range.
#' @param chrom   Chromosome numerical code.
#' @param pos_min Start reference position, with the first base having position 0.
#' @param pos_max End reference position, with the first base having position 0.
#' @useDynLib variantkey R_variantkey_range
#' @export
VariantKeyRange <- function(chrom, pos_min, pos_max) {
  n <- length(chrom)
  if ((n != length(pos_min)) || (n != length(pos_max))) {
    stop(vk.env$ERR_INPUT_LENGTH)
  }
  min <- uint64(n)
  max <- uint64(n)
  return(.Call("R_variantkey_range", as.integer(chrom), vk.env$as_uint32_bits(pos_min, "pos_min"), vk.env$as_uint32_bits(pos_max, "pos_max"), min, max))
}

#' Compares two VariantKeys by CHROM only.
#' @param vka  The first VariantKey to be compared.
#' @param vkb  The second VariantKey to be compared.
#' @return -1 if the first chromosome is smaller than the second, 0 if they are equal and 1 if the first is greater than the second.
#' @useDynLib   variantkey R_compare_variantkey_chrom
#' @export
CompareVariantKeyChrom <- function(vka, vkb) {
  n <- length(vka)
  if (n != length(vkb)) {
    stop(vk.env$ERR_INPUT_LENGTH)
  }
  ret <- integer(n)
  return(.Call("R_compare_variantkey_chrom", as.uint64(vka), as.uint64(vkb), ret))
}

#' Compares two VariantKeys by CHROM and POS.
#' @param vka  The first VariantKey to be compared.
#' @param vkb  The second VariantKey to be compared.
#' @return -1 if the first CHROM+POS is smaller than the second, 0 if they are equal and 1 if the first is greater than the second.
#' @useDynLib   variantkey R_compare_variantkey_chrom_pos
#' @export
CompareVariantKeyChromPos <- function(vka, vkb) {
  n <- length(vka)
  if (n != length(vkb)) {
    stop(vk.env$ERR_INPUT_LENGTH)
  }
  ret <- integer(n)
  return(.Call("R_compare_variantkey_chrom_pos", as.uint64(vka), as.uint64(vkb), ret))
}

#' Returns a VariantKey as a 16 character hexadecimal string.
#' @param vk  VariantKey code.
#' @useDynLib   variantkey R_variantkey_hex
#' @export
VariantKeyHex <- function(vk) {
  ret <- character(length(vk))
  return(.Call("R_variantkey_hex", as.uint64(vk), ret))
}

#' Parses a 16 character hexadecimal string into a VariantKey.
#' @param hex  VariantKey hexadecimal string (it must contain 16 hexadecimal characters).
#' @useDynLib   variantkey R_parse_variantkey_hex
#' @export
ParseVariantKeyHex <- function(hex) {
  ret <- uint64(length(hex))
  return(.Call("R_parse_variantkey_hex", as.character(hex), ret))
}

# --- BINSEARCH ---

#' Unmaps and closes a memory mapped file.
#' On success, munmap() returns 0, on failure -1.
#' @param mf Descriptor of memory-mapped file.
#' @useDynLib   variantkey R_munmap_binfile
#' @export
MunmapBinfile <- function(mf) {
  return(.Call("R_munmap_binfile", mf))
}

# --- RSIDVAR ---

#' Memory maps the RSVK binary file (rsvk.bin).
#' @param file  Path to the file to map.
#' @param ctbytes List containing the number of bytes for each column type (i.e. 1 for uint8, 2 for uint16, 4 for uint32, 8 for uint64)
#' @return The memory mapped file object, the columns object and the number of rows.
#' @useDynLib   variantkey R_mmap_rsvk_file
#' @export
MmapRSVKFile <- function(file, ctbytes) {
  return(.Call("R_mmap_rsvk_file", file, ctbytes))
}

#' Memory maps the VKRS binary file (vkrs.bin).
#' @param file  Path to the file to map.
#' @param ctbytes List containing the number of bytes for each column type (i.e. 1 for uint8, 2 for uint16, 4 for uint32, 8 for uint64)
#' @return The memory mapped file object, the columns object and the number of rows.
#' @useDynLib   variantkey R_mmap_vkrs_file
#' @export
MmapVKRSFile <- function(file, ctbytes) {
  return(.Call("R_mmap_vkrs_file", file, ctbytes))
}

#' Returns the first VariantKey associated with an rsID.
#' @param rsid    rsID to search.
#' @param first   First element of the range to search (min value = 0).
#' @param last    Element (up to but not including) where to end the search (max value = nitems).
#' @param mc    Memory-mapped columns object as returned by MmapRSVKfile.
#' @useDynLib   variantkey R_find_rv_variantkey_by_rsid
#' @export
FindRvVariantKeyByRsid <- function(rsid, first=0, last=vk.env$rsvk_$NROWS, mc=vk.env$rsvk_$MC) {
  n <- length(rsid)
  vk <- uint64(n)
  rfirst <- integer(n)
  irsid <- vk.env$as_uint32_bits(rsid, "rsid")
  return(.Call("R_find_rv_variantkey_by_rsid", mc, vk.env$as_uint32_bits(first, "first"), vk.env$as_uint32_bits(last, "last"), irsid, vk, rfirst))
}

#' Returns the next VariantKey associated with an rsID.
#' Call this in a loop after FindRVVariantKeyByRsid to get all the VariantKeys of the same rsID.
#' @param rsid    rsID to search.
#' @param pos     Current item.
#' @param last    Element (up to but not including) where to end the search (max value = nitems).
#' @param mc    Memory-mapped columns object as returned by MmapRSVKfile.
#' @useDynLib   variantkey R_get_next_rv_variantkey_by_rsid
#' @export
GetNextRvVariantKeyByRsid <- function(rsid, pos, last=vk.env$rsvk_$NROWS, mc=vk.env$rsvk_$MC) {
  n <- length(rsid)
  vk <- uint64(n)
  rpos <- integer(n)
  irsid <- vk.env$as_uint32_bits(rsid, "rsid")
  return(.Call("R_get_next_rv_variantkey_by_rsid", mc, vk.env$as_uint32_bits(pos, "pos"), vk.env$as_uint32_bits(last, "last"), irsid, vk, rpos))
}

#' Returns all the VariantKeys associated with an rsID.
#' NOTE: the output is limited to maximum 10 results.
#' @param rsid    rsID to search.
#' @param max     max number of results to return.
#' @param first   First element of the range to search (min value = 0).
#' @param last    Element (up to but not including) where to end the search (max value = nitems).
#' @param mc    Memory-mapped columns object as returned by MmapRSVKfile.
#' @useDynLib   variantkey R_find_all_rv_variantkey_by_rsid
#' @export
FindAllRvVariantKeyByRsid <- function(rsid, max=10, first=0, last=vk.env$rsvk_$NROWS, mc=vk.env$rsvk_$MC) {
  ret <- uint64(max)
  irsid <- vk.env$as_uint32_bits(rsid, "rsid")
  return(.Call("R_find_all_rv_variantkey_by_rsid", mc, vk.env$as_uint32_bits(first, "first"), vk.env$as_uint32_bits(last, "last"), irsid, ret))
}

#' Returns the first rsID associated with a VariantKey.
#' @param vk    VariantKey.
#' @param first   First element of the range to search (min value = 0).
#' @param last    Element (up to but not including) where to end the search (max value = nitems).
#' @param mc    Memory-mapped columns object as returned by MmapVKRSfile.
#' @useDynLib   variantkey R_find_vr_rsid_by_variantkey
#' @export
FindVrRsidByVariantKey <- function(vk, first=0, last=vk.env$vkrs_$NROWS, mc=vk.env$vkrs_$MC) {
  len <- length(vk)
  rsid <- integer(len)
  rfirst <- integer(len)
  return(.Call("R_find_vr_rsid_by_variantkey", mc, vk.env$as_uint32_bits(first, "first"), vk.env$as_uint32_bits(last, "last"), as.uint64(vk), rsid, rfirst))
}

#' Returns the next rsID associated with a VariantKey.
#' Call this in a loop after FindVRRsidByVariantKey to get all the rsIDs of the same VariantKey.
#' @param vk    VariantKey to search.
#' @param pos     Current item.
#' @param last    Element (up to but not including) where to end the search (max value = nitems).
#' @param mc    Memory-mapped columns object as returned by MmapRSVKfile.
#' @useDynLib   variantkey R_get_next_vr_rsid_by_variantkey
#' @export
GetNextVrRsidByVariantKey <- function(vk, pos, last=vk.env$vkrs_$NROWS, mc=vk.env$vkrs_$MC) {
  n <- length(vk)
  rsid <- integer(n)
  rpos <- integer(n)
  return(.Call("R_get_next_vr_rsid_by_variantkey", mc, vk.env$as_uint32_bits(pos, "pos"), vk.env$as_uint32_bits(last, "last"), as.uint64(vk), rsid, rpos))
}

#' Returns all the rsIDs associated with a VariantKey.
#' NOTE: the output is limited to maximum 10 results.
#' @param vk    VariantKey to search.
#' @param max     max number of results to return.
#' @param first   First element of the range to search (min value = 0).
#' @param last    Element (up to but not including) where to end the search (max value = nitems).
#' @param mc    Memory-mapped columns object as returned by MmapRSVKfile.
#' @useDynLib   variantkey R_find_all_vr_rsid_by_variantkey
#' @export
FindAllVrRsidByVariantKey <- function(vk, max=10, first=0, last=vk.env$vkrs_$NROWS, mc=vk.env$vkrs_$MC) {
  ret <- integer(max)
  return(.Call("R_find_all_vr_rsid_by_variantkey", mc, vk.env$as_uint32_bits(first, "first"), vk.env$as_uint32_bits(last, "last"), as.uint64(vk), ret))
}

#' Returns the first rsID of a CHROM and POS range.
#' @param chrom   Chromosome encoded number.
#' @param pos_min   Start reference position, with the first base having position 0.
#' @param pos_max   End reference position, with the first base having position 0.
#' @param first   First element of the range to search (min value = 0).
#' @param last    Element (up to but not including) where to end the search (max value = nitems).
#' @param mc    Memory-mapped columns object as returned by MmapVKRSfile.
#' @useDynLib   variantkey R_find_vr_chrompos_range
#' @export
FindVrChromposRange <- function(chrom, pos_min, pos_max, first=0, last=vk.env$vkrs_$NROWS, mc=vk.env$vkrs_$MC) {
  n <- length(chrom)
  if ((n != length(pos_min)) || (n != length(pos_max))) {
    stop(vk.env$ERR_INPUT_LENGTH)
  }
  rsid <- integer(n)
  rfirst <- integer(n)
  rlast <- integer(n)
  ichrom <- vk.env$as_uint32_bits(chrom, "chrom")
  ipos_min <- vk.env$as_uint32_bits(pos_min, "pos_min")
  ipos_max <- vk.env$as_uint32_bits(pos_max, "pos_max")
  return(.Call("R_find_vr_chrompos_range", mc, vk.env$as_uint32_bits(first, "first"), vk.env$as_uint32_bits(last, "last"), ichrom, ipos_min, ipos_max, rsid, rfirst, rlast))
}

# --- NRVK ---

#' Memory maps the NRVK binary file (nrvk.bin).
#' @param file  Path to the file to map.
#' @return The memory mapped file object, the columns object and the number of rows.
#' @useDynLib   variantkey R_mmap_nrvk_file
#' @export
MmapNRVKFile <- function(file) {
  return(.Call("R_mmap_nrvk_file", file))
}

#' Looks up the REF and ALT strings of a VariantKey.
#' @param vk     VariantKey to search.
#' @param mc     Memory-mapped columns object as returned by MmapNRVKfile.
#' @return REF+ALT length or 0 if the VariantKey is not found.
#' @useDynLib   variantkey R_find_ref_alt_by_variantkey
#' @export
FindRefAltByVariantKey <- function(vk, mc=vk.env$nrvk_$MC) {
  n <- length(vk)
  ref <- character(n)
  alt <- character(n)
  return(.Call("R_find_ref_alt_by_variantkey", mc, as.uint64(vk), ref, alt))
}

#' Reverses a VariantKey into its CHROM, POS, REF and ALT components.
#' @param vk     VariantKey code.
#' @param mc     Memory-mapped columns object as returned by MmapNRVKfile.
#' @useDynLib   variantkey R_reverse_variantkey
#' @export
ReverseVariantKey <- function(vk, mc=vk.env$nrvk_$MC) {
  n <- length(vk)
  chrom <- character(n)
  pos <- integer(n)
  ref <- character(n)
  alt <- character(n)
  return(.Call("R_reverse_variantkey", mc, as.uint64(vk), chrom, pos, ref, alt))
}

#' Returns the REF length of a VariantKey.
#' @param vk     VariantKey.
#' @param mc     Memory-mapped columns object as returned by MmapNRVKfile.
#' @return REF length or 0 if the VariantKey is not reversible and not found.
#' @useDynLib   variantkey R_get_variantkey_ref_length
#' @export
GetVariantKeyRefLength <- function(vk, mc=vk.env$nrvk_$MC) {
  ret <- integer(length(vk))
  return(.Call("R_get_variantkey_ref_length", mc, as.uint64(vk), ret))
}

#' Returns the end position of a VariantKey (POS + REF length).
#' @param vk     VariantKey.
#' @param mc     Memory-mapped columns object as returned by MmapNRVKfile.
#' @return Variant end position.
#' @useDynLib   variantkey R_get_variantkey_endpos
#' @export
GetVariantKeyEndPos <- function(vk, mc=vk.env$nrvk_$MC) {
  ret <- integer(length(vk))
  return(.Call("R_get_variantkey_endpos", mc, as.uint64(vk), ret))
}

#' Returns the CHROM and START POS section of a VariantKey.
#' @param vk     VariantKey.
#' @useDynLib   variantkey R_get_variantkey_chrom_startpos
#' @export
GetVariantKeyChromStartPos <- function(vk) {
  ret <- uint64(length(vk))
  return(.Call("R_get_variantkey_chrom_startpos", as.uint64(vk), ret))
}

#' Returns the CHROM and END POS of a VariantKey.
#' @param vk     VariantKey.
#' @param mc     Memory-mapped columns object as returned by MmapNRVKfile.
#' @useDynLib   variantkey R_get_variantkey_chrom_endpos
#' @export
GetVariantKeyChromEndPos <- function(vk, mc=vk.env$nrvk_$MC) {
  ret <- uint64(length(vk))
  return(.Call("R_get_variantkey_chrom_endpos", mc, as.uint64(vk), ret))
}

#' Writes the content of the NRVK memory mapped file as a TSV file.
#' @param tsvfile  Output tsv file name. Note that existing files will be replaced.
#' @param mc     Memory-mapped columns object as returned by MmapNRVKfile.
#' @return Number of written bytes or 0 in case of error.
#'   For the reverse operation see the resources/tools/nrvk.sh script.
#' @useDynLib   variantkey R_nrvk_bin_to_tsv
#' @export
VknrBinToTsv <- function(tsvfile, mc=vk.env$nrvk_$MC) {
  return(.Call("R_nrvk_bin_to_tsv", mc, tsvfile))
}

# --- GENOREF ---

#' Memory maps the genoref binary file (fasta.bin).
#' @param file  Path to the file to map.
#' @return The memory mapped file object, the columns object and the number of rows.
#' @useDynLib   variantkey R_mmap_genoref_file
#' @export
MmapGenorefFile <- function(file) {
  return(.Call("R_mmap_genoref_file", file))
}

#' Returns the genome reference nucleotide at the given chromosome and position.
#' @param chrom   Encoded Chromosome number (see encode_chrom).
#' @param pos   Position. The reference position, with the first base having position 0.
#' @param mf    Memory-mapped file object as returned by MmapGenorefFile.
#' @useDynLib   variantkey R_get_genoref_seq
#' @export
GetGenorefSeq <- function(chrom, pos, mf=vk.env$genoref_$MF) {
  n <- length(chrom)
  if (n != length(pos)) {
    stop(vk.env$ERR_INPUT_LENGTH)
  }
  ret <- integer(n)
  return(intToUtf8(.Call("R_get_genoref_seq", mf, as.integer(chrom), vk.env$as_uint32_bits(pos, "pos"), ret), multiple = TRUE))
}

#' Checks a reference allele against the genome reference data.
#' @param chrom   Encoded Chromosome number (see encode_chrom).
#' @param pos   Position. The reference position, with the first base having position 0.
#' @param ref   Reference allele. String containing a sequence of nucleotide letters.
#' @param mf    Memory-mapped file object as returned by MmapGenorefFile.
#' @return Positive number in case of success, negative in case of error:
#'   *  0 the reference allele matches the reference genome;
#'   *  1 the reference allele is inconsistent with the genome reference (i.e. when contains nucleotide letters other than A, C, G and T);
#'   * -1 the reference allele does not match the reference genome;
#'   * -2 the reference allele is longer than the genome reference sequence.
#' @useDynLib   variantkey R_check_reference
#' @export
CheckReference <- function(chrom, pos, ref, mf=vk.env$genoref_$MF) {
  n <- length(chrom)
  if ((n != length(pos)) || (n != length(ref))) {
    stop(vk.env$ERR_INPUT_LENGTH)
  }
  ret <- integer(n)
  return(.Call("R_check_reference", mf, as.integer(chrom), vk.env$as_uint32_bits(pos, "pos"), as.character(ref), ret))
}

#' Replaces each nucleotide of an allele with its complement.
#' The resulting string is always in uppercase.
#' Support extended nucleotide letters.
#' @param allele  Allele. String containing a sequence of nucleotide letters.
#' @param size  Length of the allele string.
#' @useDynLib   variantkey R_flip_allele
#' @export
FlipAllele <- function(allele) {
  ret <- character(length(allele))
  return(.Call("R_flip_allele", as.character(allele), ret))
}

#' Normalizes a variant against the genome reference, flipping the alleles if required.
#' See the normalization algorithm described at:
#' https://genome.sph.umich.edu/wiki/Variant_Normalization
#' @param chrom    Chromosome encoded number.
#' @param pos    Position. The reference position, with the first base having position 0.
#' @param ref    Reference allele. String containing a sequence of nucleotide letters.
#' @param alt    Alternate non-reference allele string.
#' @param mf    Memory-mapped file object as returned by MmapGenorefFile.
#' @return Positive bitmask number in case of success, negative number in case of error.
#'   When positive, each bit set has a different meaning, as defined by the NORM_* codes:
#'   * bit 0:
#'   Reference allele is inconsistent with the genome reference (i.e. when contains nucleotide letters other than A, C, G and T).
#'   * bit 1:
#'   Alleles have been swapped.
#'   * bit 2:
#'   Allele nucleotides have been flipped (each nucleotide has been replaced with its complement).
#'   * bit 3:
#'   Alleles have been left extended.
#'   * bit 4:
#'   Alleles have been right trimmed.
#'   * bit 5:
#'   Alleles have been left trimmed.
#' @useDynLib   variantkey R_normalize_variant
#' @export
NormalizeVariant <- function(chrom, pos, ref, alt, mf=vk.env$genoref_$MF) {
  n <- length(chrom)
  if ((n != length(pos)) || (n != length(ref)) || (n != length(alt))) {
    stop(vk.env$ERR_INPUT_LENGTH)
  }
  rcode <- integer(n)
  rpos <- integer(n)
  rref <- character(n)
  ralt <- character(n)
  return(.Call("R_normalize_variant", mf, as.integer(chrom), vk.env$as_uint32_bits(pos, "pos"), as.character(ref), as.character(alt), rcode, rpos, rref, ralt))
}

#' Normalizes a variant and returns its VariantKey.
#' @param chrom    Chromosome encoded number.
#' @param pos    Position. The reference position.
#' @param posindex   Position index: 0 for 0-based, 1 for 1-based.
#' @param ref    Reference allele. String containing a sequence of nucleotide letters.
#' @param alt    Alternate non-reference allele string.
#' @param mf    Memory-mapped file object as returned by MmapGenorefFile.
#' @useDynLib   variantkey R_normalized_variantkey
#' @export
NormalizedVariantKey <- function(chrom, pos, posindex, ref, alt, mf=vk.env$genoref_$MF) {
  n <- length(chrom)
  if ((n != length(pos)) || (n != length(ref)) || (n != length(alt))) {
    stop(vk.env$ERR_INPUT_LENGTH)
  }
  rvk <- uint64(n)
  rcode <- integer(n)
  return(.Call("R_normalized_variantkey", mf, as.character(chrom), vk.env$as_uint32_bits(pos, "pos"), as.integer(posindex), as.character(ref), as.character(alt), rvk, rcode))
}

# --- REGIONKEY ---

#' Encodes a strand direction: -1 to 2, 0 to 0, +1 to 1.
#' @param strand   Strand direction (-1, 0, +1).
#' @useDynLib   variantkey R_encode_region_strand
#' @export
EncodeRegionStrand <- function(strand) {
  ret <- integer(length(strand))
  return(.Call("R_encode_region_strand", as.integer(strand), ret))
}

#' Decodes a strand code: 0 to 0, 1 to +1, 2 to -1.
#' @param code   Strand code.
#' @useDynLib   variantkey R_decode_region_strand
#' @export
DecodeRegionStrand <- function(strand) {
  ret <- integer(length(strand))
  return(.Call("R_decode_region_strand", as.integer(strand), ret))
}

#' Assembles a RegionKey from its pre-encoded components.
#' @param chrom    Encoded Chromosome (see encode_chrom).
#' @param startpos   Start position (zero based).
#' @param endpos   End position (startpos + region_length).
#' @param strand   Encoded Strand direction (-1 > 2, 0 > 0, +1 > 1)
#' @useDynLib   variantkey R_encode_regionkey
#' @export
EncodeRegionKey <- function(chrom, startpos, endpos, strand) {
  n <- length(chrom)
  if ((n != length(startpos)) || (n != length(endpos)) || (n != length(strand))) {
    stop(vk.env$ERR_INPUT_LENGTH)
  }
  ret <- uint64(n)
  return(.Call("R_encode_regionkey", as.integer(chrom), vk.env$as_uint32_bits(startpos, "startpos"), vk.env$as_uint32_bits(endpos, "endpos"), as.integer(strand), ret))
}

#' Extracts the CHROM code from a RegionKey.
#' @param rk RegionKey code.
#' @useDynLib   variantkey R_extract_regionkey_chrom
#' @export
ExtractRegionKeyChrom <- function(rk) {
  ret <- integer(length(rk))
  return(.Call("R_extract_regionkey_chrom", as.uint64(rk), ret))
}

#' Extracts the START POS value from a RegionKey.
#' @param rk RegionKey code.
#' @useDynLib   variantkey R_extract_regionkey_startpos
#' @export
ExtractRegionKeyStartPos <- function(rk) {
  ret <- integer(length(rk))
  return(.Call("R_extract_regionkey_startpos", as.uint64(rk), ret))
}

#' Extracts the END POS value from a RegionKey.
#' @param rk RegionKey code.
#' @useDynLib   variantkey R_extract_regionkey_endpos
#' @export
ExtractRegionKeyEndPos <- function(rk) {
  ret <- integer(length(rk))
  return(.Call("R_extract_regionkey_endpos", as.uint64(rk), ret))
}

#' Extracts the STRAND code from a RegionKey.
#' @param rk RegionKey code.
#' @useDynLib   variantkey R_extract_regionkey_strand
#' @export
ExtractRegionKeyStrand <- function(rk) {
  ret <- integer(length(rk))
  return(.Call("R_extract_regionkey_strand", as.uint64(rk), ret))
}

#' Splits a RegionKey into its encoded components.
#' @param rk RegionKey code.
#' @useDynLib   variantkey R_decode_regionkey
#' @export
DecodeRegionKey <- function(rk) {
  n <- length(rk)
  chrom <- integer(n)
  startpos <- integer(n)
  endpos <- integer(n)
  strand <- integer(n)
  return(.Call("R_decode_regionkey", as.uint64(rk), chrom, startpos, endpos, strand))
}

#' Reverses a RegionKey into its decoded components.
#' @param rk     RegionKey code.
#' @useDynLib   variantkey R_reverse_regionkey
#' @export
ReverseRegionKey <- function(rk) {
  n <- length(rk)
  chrom <- character(n)
  startpos <- integer(n)
  endpos <- integer(n)
  strand <- integer(n)
  return(.Call("R_reverse_regionkey", as.uint64(rk), chrom, startpos, endpos, strand))
}

#' Returns a RegionKey for the given CHROM, START POS (0-based), END POS and STRAND.
#' @param chrom    Chromosome. An identifier from the reference genome, no white-space or leading zeros permitted.
#' @param startpos   Start position (zero based).
#' @param endpos   End position (startpos + region_length).
#' @param strand   Strand direction (-1, 0, +1)
#' @useDynLib   variantkey R_regionkey
#' @export
RegionKey <- function(chrom, startpos, endpos, strand) {
  n <- length(chrom)
  if ((n != length(startpos)) || (n != length(endpos)) || (n != length(strand))) {
    stop(vk.env$ERR_INPUT_LENGTH)
  }
  ret <- uint64(n)
  return(.Call("R_regionkey", as.character(chrom), vk.env$as_uint32_bits(startpos, "startpos"), vk.env$as_uint32_bits(endpos, "endpos"), as.integer(strand), ret))
}

#' Extends a RegionKey region by a fixed amount at both ends.
#' @param rk     RegionKey code.
#' @param size   Amount to extend the region.
#' @useDynLib   variantkey R_extend_regionkey
#' @export
ExtendRegionKey <- function(rk, size) {
  n <- length(rk)
  ret <- uint64(n)
  # The amount is recycled to the length of rk, so every region is extended by
  # its own amount.
  return(.Call("R_extend_regionkey", as.uint64(rk), rep_len(as.integer(size), n), ret))
}

#' Returns a RegionKey as a 16 character hexadecimal string.
#' @param vk  RegiontKey code.
#' @useDynLib   variantkey R_regionkey_hex
#' @export
RegionKeyHex <- function(vk) {
  ret <- character(length(vk))
  return(.Call("R_regionkey_hex", as.uint64(vk), ret))
}

#' Parses a 16 character hexadecimal string into a RegionKey.
#' @param hex  RegionKey hexadecimal string (it must contain 16 hexadecimal characters).
#' @useDynLib   variantkey R_parse_regionkey_hex
#' @export
ParseRegionKeyHex <- function(hex) {
  ret <- uint64(length(hex))
  return(.Call("R_parse_regionkey_hex", as.character(hex), ret))
}

#' Returns the CHROM and START POS section of a RegionKey.
#' @param rk RegionKey code.
#' @useDynLib   variantkey R_get_regionkey_chrom_startpos
#' @export
GetRegionKeyChromStartPos <- function(rk) {
  ret <- uint64(length(rk))
  return(.Call("R_get_regionkey_chrom_startpos", as.uint64(rk), ret))
}

#' Returns the CHROM and END POS of a RegionKey.
#' @param rk RegionKey code.
#' @useDynLib   variantkey R_get_regionkey_chrom_endpos
#' @export
GetRegionKeyChromEndPos <- function(rk) {
  ret <- uint64(length(rk))
  return(.Call("R_get_regionkey_chrom_endpos", as.uint64(rk), ret))
}

#' Checks whether two regions overlap.
#' @param a_chrom   Region A chromosome code.
#' @param a_startpos  Region A start position.
#' @param a_endpos  Region A end position (startpos + region length).
#' @param b_chrom   Region B chromosome code.
#' @param b_startpos  Region B start position.
#' @param b_endpos  Region B end position (startpos + region length).
#' @return 1 if the regions overlap, 0 otherwise.
#' @useDynLib   variantkey R_are_overlapping_regions
#' @export
AreOverlappingRegions <- function(a_chrom, a_startpos, a_endpos, b_chrom, b_startpos, b_endpos) {
  n <- length(a_chrom)
  if ((n != length(a_startpos)) || (n != length(a_endpos)) || (n != length(b_chrom)) || (n != length(b_startpos)) || (n != length(b_endpos))) {
    stop(vk.env$ERR_INPUT_LENGTH)
  }
  ret <- integer(n)
  return(.Call("R_are_overlapping_regions", as.integer(a_chrom), vk.env$as_uint32_bits(a_startpos, "a_startpos"), vk.env$as_uint32_bits(a_endpos, "a_endpos"), as.integer(b_chrom), vk.env$as_uint32_bits(b_startpos, "b_startpos"), vk.env$as_uint32_bits(b_endpos, "b_endpos"), ret))
}

#' Checks whether a region and a RegionKey overlap.
#' @param chrom   Region A chromosome code.
#' @param startpos  Region A start position.
#' @param endpos  Region A end position (startpos + region length).
#' @param rk    RegionKey or region B.
#' @return 1 if the regions overlap, 0 otherwise.
#' @useDynLib   variantkey R_are_overlapping_region_regionkey
#' @export
AreOverlappingRegionRegionKey <- function(chrom, startpos, endpos, rk) {
  n <- length(chrom)
  if ((n != length(startpos)) || (n != length(endpos)) || (n != length(rk))) {
    stop(vk.env$ERR_INPUT_LENGTH)
  }
  ret <- integer(n)
  return(.Call("R_are_overlapping_region_regionkey", as.integer(chrom), vk.env$as_uint32_bits(startpos, "startpos"), vk.env$as_uint32_bits(endpos, "endpos"), as.uint64(rk), ret))
}

#' Checks whether two RegionKeys overlap.
#' @param rka    RegionKey A.
#' @param rkb    RegionKey B.
#' @return 1 if the regions overlap, 0 otherwise.
#' @useDynLib   variantkey R_are_overlapping_regionkeys
#' @export
AreOverlappingRegionKeys <- function(rka, rkb) {
  n <- length(rka)
  if (n != length(rkb)) {
    stop(vk.env$ERR_INPUT_LENGTH)
  }
  ret <- integer(n)
  return(.Call("R_are_overlapping_regionkeys", as.uint64(rka), as.uint64(rkb), ret))
}

#' Checks whether a VariantKey and a RegionKey overlap.
#' @param vk  VariantKey code.
#' @param rk  RegionKey code.
#' @param mc  Memory-mapped columns object as returned by MmapNRVKfile.
#' @return 1 if the regions overlap, 0 otherwise.
#' @useDynLib   variantkey R_are_overlapping_variantkey_regionkey
#' @export
AreOverlappingVariantKeyRegionKey <- function(vk, rk, mc=vk.env$nrvk_$MC) {
  n <- length(vk)
  if (n != length(rk)) {
    stop(vk.env$ERR_INPUT_LENGTH)
  }
  ret <- integer(n)
  return(.Call("R_are_overlapping_variantkey_regionkey", mc, as.uint64(vk), as.uint64(rk), ret))
}

#' Converts a VariantKey into a RegionKey.
#' @param vk  VariantKey code.
#' @param mc  Memory-mapped columns object as returned by MmapNRVKfile.
#' @useDynLib   variantkey R_variantkey_to_regionkey
#' @export
VariantToRegionkey <- function(vk, mc=vk.env$nrvk_$MC) {
  ret <- uint64(length(vk))
  return(.Call("R_variantkey_to_regionkey", mc, as.uint64(vk), ret))
}

# --- ESID ---

#' Encodes up to 10 characters of a string into a 64 bit unsigned integer.
#' This function can be used to convert generic string IDs to numeric IDs.
#' @param str  The string to encode. The characters beyond the first 10 from start are ignored. It supports ASCII characters from '!' to 'z'.
#' @param start  First character to encode, starting from 0. To encode the last 10 characters, set this value at (size - 10).
#' @useDynLib   variantkey R_encode_string_id
#' @export
EncodeStringID <- function(str, start=0) {
  n <- length(str)
  m <- length(start)
  if ((m > 1) && (n > m)) {
    stop(vk.env$ERR_INPUT_LENGTH)
  }
  ret <- uint64(n)
  return(.Call("R_encode_string_id", as.character(str), as.integer(start), ret))
}

#' Encodes a string made of a character section, a separator and a numerical section
#' into a 64 bit unsigned integer. For example: "ABCDE:0001234"
#' Encodes up to 5 characters in uppercase, a number up to 2^27, and up to 7 zero padding digits.
#' Strings of 10 characters or less are encoded as by EncodeStringID().
#' @param str  The string to encode. It supports ASCII characters from '!' to 'z'.
#' @param sep  Separator character between string and number.
#' @useDynLib   variantkey R_encode_string_num_id
#' @export
EncodeStringNumID <- function(str, sep=":") {
  ret <- uint64(length(str))
  return(.Call("R_encode_string_num_id", as.character(str), utf8ToInt(as.character(sep))[1], ret))
}

#' Decodes an encoded string ID.
#' This function is the reverse of encode_string_id.
#' The string is always returned in uppercase mode.
#' @param esid   Encoded string ID code.
#' @useDynLib   variantkey R_decode_string_id
#' @export
DecodeStringID <- function(esid) {
  ret <- character(length(esid))
  return(.Call("R_decode_string_id", as.uint64(esid), ret))
}

#' Hashes a string into a non-reversible 64 bit string ID.
#' This function can be used to convert long string IDs to numeric IDs.
#' @param str  The string to encode.
#' @useDynLib   variantkey R_hash_string_id
#' @export
HashStringID <- function(str) {
  ret <- uint64(length(str))
  return(.Call("R_hash_string_id", as.character(str), ret))
}
