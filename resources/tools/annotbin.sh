#!/usr/bin/env bash
#
# annotbin.sh
#
# Convert a variant annotation table into a VariantKey-indexed binary lookup
# table, by decompressing the input and piping it to the vkbin tool.
#
# The input is a tab-separated file whose first four columns are CHROM, POS
# (1-based), REF and ALT, optionally compressed with bgzip or gzip. The output
# is a BINSRC1 file whose first column is the VariantKey, followed by one
# unsigned integer column per value column.
#
# Requires:
#  - vkbin (built from c/vkbin, see "make build" in the c directory)
#  - bgzip or gzip, only when the input is compressed
#
# Nicola Asuni
# ------------------------------------------------------------------------------

# OUTPUT binary format:
#    BINSRC1 00
#    [1 BYTE NUMBER OF COLUMNS][1 BYTE ITEM SIZE PER COLUMN]+[PADDING]
#    [8 BYTE NUMBER OF ROWS]
#    [8 BYTE COLUMN OFFSET]+
#    [8 BYTE VARIANTKEY COLUMN]+
#    [VALUE COLUMN]+

set -e -u -o pipefail -o errtrace

: ${ANNOT_INPUT_FILE:?}                  # Input TSV file, optionally bgzip or gzip compressed
: ${ANNOT_OUTPUT_FILE:=annot.bin}        # Name of the output binary file
: ${ANNOT_COLUMNS:?}                     # Column specifications, see below
: ${ANNOT_SKIP_LINES:=1}                 # Number of leading header lines to skip
: ${VKBIN:=vkbin}                        # Path of the vkbin tool

# Each entry of ANNOT_COLUMNS describes one value column as
# WIDTH:DECIMALS[:OFFSET][:NA]
#
#   WIDTH     stored size in bytes: 1, 2, 4 or 8
#   DECIMALS  decimal places to preserve, the value is scaled by 10^DECIMALS
#   OFFSET    decimal added before scaling, to shift a signed range positive
#   NA        store the largest value of WIDTH for a missing field
#
# OFFSET and NA follow DECIMALS in any order and each may appear once.
#
# For example, the AlphaGenome AVI SNV scores carry a raw_score spanning about
# -1.3 to +4.6 and a PHRED score from 0, both with 5 decimals:
#
#   ANNOT_INPUT_FILE=alphagenome_variant_impact_score_snvs.tsv.gz \
#   ANNOT_OUTPUT_FILE=avi.bin \
#   ANNOT_COLUMNS="4:5:2.0 4:5:0" \
#   ./annotbin.sh

# Pick the decompressor from the file name. bgzip is preferred when present
# because it is the format the annotation files are usually distributed in,
# but gzip reads a bgzip file just as well.
case "${ANNOT_INPUT_FILE}" in
    *.gz|*.bgz)
        if command -v bgzip > /dev/null 2>&1; then
            READER="bgzip -dc"
        else
            READER="gzip -dc"
        fi
        ;;
    *)
        READER="cat"
        ;;
esac

${READER} "${ANNOT_INPUT_FILE}" \
| ${VKBIN} -o "${ANNOT_OUTPUT_FILE}" -s "${ANNOT_SKIP_LINES}" ${ANNOT_COLUMNS}
