# Tools

Scripts to normalize VCF files and generate VariantKey information.


* **fastabin.sh**
  * Create a binary version of the input reference genome sequence FASTA file for quick lookup.
    It only extracts the first 25 sequences for chromosomes 1 to 22, X, Y and MT.

* **vcfnorm.sh**
  * Normalize VCF files (allele decomposition + normalization)
  * *Requires*:
    * vt (https://github.com/atks/vt)
    * tabix

* **annotbin.sh**
  * Convert a variant annotation table (TSV with CHROM, POS, REF, ALT and value
    columns) into a VariantKey-indexed BINSRC1 binary lookup table.
    Decompresses the input and pipes it to the `vkbin` tool.
  * *Requires*:
    * vkbin (built from `c/vkbin`)
    * bgzip or gzip, only when the input is compressed

* **vkhexbin.sh**
  * Process the variantKey HEX file to generate the final binary counterparts:
    * <FILE>.vcf.gz     : decomposed and normalized VCF file with added VariantKey.
    * <FILE>.vcf.gz.tbi : VCF file index.
    * vkrs.bin          : VariantKey to rsID binary lookup table.
    * rsvk.bin          : rsID to VariantKey binary lookup table.
    * nrvk.bin          : Non-reversible VariantKey to REF+ALT lookup table.
  * *Requires*:
    * vt      (https://github.com/atks/vt)
    * bcftool (https://github.com/samtools/bcftools/tree/develop)
    * sort    (coreutils)
    * xxd     (vim-common)

* **rsvk.sh**
  * Process a rsID and VariantKey hexadecimal TSV file to generate the
    rsID to VariantKey binary lookup table: rsvk.bin.
    Sourced by vkhexbin.sh.

* **vkrs.sh**
  * Process a VariantKey and rsID hexadecimal TSV file to generate the
    VariantKey to rsID binary lookup table: vkrs.bin.
    Sourced by vkhexbin.sh.

* **nrvk.sh**
  * Process a non-reversible VariantKey TSV file to generate the REF and ALT
    binary lookup table: nrvk.bin.
    Sourced by vkhexbin.sh.

## NOTE:

Prebuilt binary files can be downloaded from:
https://sourceforge.net/projects/variantkey/files/
