# VariantKey

This software library provides:

* ***VariantKey***: a reversible numerical encoding schema for human genetic variants.
* ***RegionKey***: a reversible numerical encoding schema for human genomic regions.
* ***ESID***: a reversible numerical encoding schema for genetic string identifiers.
* ***normalize_variant***: a function to normalize human genetic variants for a given genome reference.

[![Sponsor on GitHub](https://img.shields.io/badge/sponsor-github-EA4AAA.svg?logo=githubsponsors&logoColor=white)](https://github.com/sponsors/tecnickcom)

If this project is useful to you, please consider [supporting development via GitHub Sponsors](https://github.com/sponsors/tecnickcom).

[![check](https://github.com/tecnickcom/variantkey/actions/workflows/check.yaml/badge.svg)](https://github.com/tecnickcom/variantkey/actions/workflows/check.yaml)

* **category**    Libraries
* **author**      Nicola Asuni
* **license**     [MIT](https://github.com/tecnickcom/variantkey/blob/main/LICENSE)
* **link**        https://github.com/tecnickcom/variantkey

-----------------------------------------------------------------

**How to cite**

Nicola Asuni, Steven Wilder [VariantKey - A Reversible Numerical Representation of Human Genetic Variants](https://www.biorxiv.org/content/10.1101/473744v3), bioRxiv 473744; doi: [https://doi.org/10.1101/473744](https://doi.org/10.1101/473744).


-----------------------------------------------------------------

## TOC

* [Description](#description)
* [Quick Start](#quickstart)
* [Human Genetic Variant Definition](#hgvdefinition)
* [Variant Decomposition and Normalization](#decompandnorm)
    * [Decomposition](#decomposition)
    * [Normalization](#normalization)
        * [Normalization Function](#normfunc)
* **[VariantKey Format](#vkformat)**
    * [VariantKey Properties](#vkproperties)
* [VariantKey Input values](#vkinput)
* **[RegionKey](#regionkey)**
    * [RegionKey Properties](#rkproperties)
* [Encoding String IDs](#esid)
* [Binary files for lookup tables](#binaryfiles)
* **[Annotation lookup tables](#annotations)**
    * [Why convert an annotation file](#whyconvert)
    * [The vkbin tool](#vkbintool)
    * [Worked example: AlphaGenome AVI scores](#aviexample)
    * [Reading the output](#readoutput)
    * [Choosing column widths](#colwidths)
* [C Library](#clib)
* [Go Library](#golib)
* [Python Module](#pythonlib)
* [Python Vectorized Class](#pythonclass)
* [R Module](#rlib)
* [Javascript library](#jslib)

-----------------------------------------------------------------

<a name="description"></a>
## Description

Human genetic variants are usually represented by four values with variable length: chromosome, position, reference and alternate alleles. There is no guarantee that these components are represented in a consistent way across different data sources, and processing variant-based data can be inefficient because four different comparison operations are needed for each variant, three of which are string comparisons. Working with strings, in contrast to numbers, poses extra challenges on computer memory allocation and data-representation. Existing variant identifiers do not typically represent every possible variant we may be interested in, and they are not directly reversible.

**VariantKey**, a novel reversible numerical encoding schema for human genetic variants, overcomes these limitations by encoding each variant as a single 64 bit number that can still be searched and sorted per chromosome and position.

The individual components of short variants (up to 11 bases between `REF` and `ALT` alleles) can be directly read back from the VariantKey, while long variants require a lookup table to retrieve the reference and alternate allele strings.

The [VariantKey Format](#vkformat) doesn't represent universal codes, it only encodes normalized `CHROM`, `POS`, `REF` and `ALT`, so each code is unique for a given reference genome. The direct comparison of two VariantKeys makes sense only if they both refer to the same genome reference.

----------

<a name="quickstart"></a>
## Quick Start

This project includes a Makefile that allows you to test and build the project in a Linux-compatible system with simple commands.

To see all available options, from the project root type:

```
make help
```

To build all the VariantKey versions inside a Docker container (requires Docker):

```
make dbuild
```

An arbitrary make target can be executed inside a [Docker](https://www.docker.com/) container by specifying the `MAKETARGET` parameter:

```
MAKETARGET='build' make dbuild
```
The list of make targets can be obtained by typing ```make```


The base Docker building environment is defined in the following Dockerfile:

```
resources/docker/Dockerfile.dev
```

To build and test only a specific language version, `cd` into the language directory and use the `make` command.
For example:

```
cd c
make test
```

----------

<a name="hgvdefinition"></a>
## Human Genetic Variant Definition

In this context, the human genetic variant for a given genome assembly is defined as the set of four components compatible with the VCF format:

* **`CHROM`** - chromosome: An identifier from the reference genome. It only has 26 valid values: autosomes from 1 to 22, the sex chromosomes X=23 and Y=24, mitochondria MT=25 and a symbol NA=0 to indicate missing data.
* **`POS`** - position: The reference position in the chromosome, with the first nucleotide having position 0. The largest expected value is less than 250 million to represent the last base pair in the chromosome 1.
* **`REF`** - reference allele: String containing a sequence of reference nucleotide letters. The value in the POS field refers to the position of the first nucleotide in the String.
* **`ALT`** - alternate allele: Single alternate non-reference allele. String containing a sequence of nucleotide letters. Multiallelic variants must be decomposed in individual biallelic variants.


<a name="decompandnorm"></a>
## Variant Decomposition and Normalization

The *VariantKey* model assumes that the variants have been decomposed and normalized.

<a name="decomposition"></a>
### Decomposition

In the common *Variant Call Format* (VCF) the alternate field can contain comma-separated strings for multiallelic variants, while in this context we only consider biallelic variants to allow for allelic comparisons between different data sets.

For example, the multiallelic variant:

```
    {CHROM=1, POS=3759889, REF=TA, ALT=TAA,TAAA,T}
```

can be decomposed as three biallelic variants:

```
    {CHROM=1, POS=3759889, REF=TA, ALT=TAA}
    {CHROM=1, POS=3759889, REF=TA, ALT=TAAA}
    {CHROM=1, POS=3759889, REF=TA, ALT=T}
```

In VCF files the decomposition from multiallelic to biallelic variants can be performed using the '[vt](https://genome.sph.umich.edu/wiki/Vt#Decompose)' software tool with the command:

```
    vt decompose -s source.vcf -o decomposed.vcf
```

The `-s` option (smart decomposition) splits up `INFO` and `GENOTYPE` fields that have number counts of `R` and `A` appropriately.


#### Example:

* input

```
  #CHROM  POS     ID   REF     ALT         QUAL   FILTER  INFO                  FORMAT    S1                                      S2
  1       3759889 .    TA      TAA,TAAA,T  .      PASS    AF=0.342,0.173,0.037  GT:DP:PL  1/2:81:281,5,9,58,0,115,338,46,116,809  0/0:86:0,30,323,31,365,483,38,291,325,567
```

* output

```
  #CHROM  POS     ID   REF     ALT         QUAL   FILTER  INFO                                                 FORMAT   S1               S2
  1       3759889 .    TA      TAA         .      PASS    AF=0.342;OLD_MULTIALLELIC=1:3759889:TA/TAA/TAAA/T    GT:PL    1/.:281,5,9      0/0:0,30,323
  1       3759889 .    TA      TAAA        .      .       AF=0.173;OLD_MULTIALLELIC=1:3759889:TA/TAA/TAAA/T    GT:PL    ./1:281,58,115   0/0:0,31,483
  1       3759889 .    TA      T           .      .       AF=0.037;OLD_MULTIALLELIC=1:3759889:TA/TAA/TAAA/T    GT:PL    ./.:281,338,809  0/0:0,38,567
```

<a name="normalization"></a>
### Normalization

A normalization step is required to ensure a consistent and unambiguous representation of variants.
As shown in the following example, there are multiple ways to represent the same variant, but only one can be considered "normalized" as defined by [Tan et al., 2015](https://doi.org/10.1093/bioinformatics/btv112):

* *A variant representation is normalized if and only if it is left aligned and parsimonious.*
* *A variant representation is left aligned if and only if its base position is smallest among all potential representations having the same allele length and representing the same variant.*
* *A variant representation is parsimonious if and only if the entry has the shortest allele length among all VCF entries representing the same variant.*

Example of entries representing the same variant:

```
                                                  DELETE
                                    POS: 0        ||
                         VARIANT    REF: GGGCACACACAGGG
                                    ALT: GGGCACACAGGG

                                    POS:      5
                  NOT-LEFT-ALIGNED  REF:      CAC
                                    ALT:      C

                                    POS:   2
NOT-LEFT-ALIGNED, NOT-PARSIMONIOUS  REF:   GCACA
                                    ALT:   GCA

                                    POS:  1
                  NOT-PARSIMONIOUS  REF:  GGCA
                                    ALT:  GG

                                    POS:   2
                      NORMALIZED    REF:   GCA
                                    ALT:   G
```

In VCF files the variant normalization can be performed using the [vt](https://genome.sph.umich.edu/wiki/Vt#Normalization) software tool with the command:

```
    vt normalize decomposed.vcf -m -r genome.fa -o normalized.vcf
```
or the [bcftools](https://samtools.github.io/bcftools/bcftools.html#norm) software with the command:

```
    bcftools norm -f genome.fa -o normalized.vcf decomposed.vcf
```

or decompose and normalize with a single command:

```
    bcftools norm --multiallelics -any -f genome.fa -o normalized.vcf source.vcf
```

<a name="normfunc"></a>
#### Normalization Function

Individual biallelic variants can be normalized using the `normalize_variant` function provided by this library.  

The `normalize_variant` function first checks if the reference allele matches the genome reference.
The match is considered valid and consistent if there is a perfect letter-by-letter match, and valid but not consistent if one or more letter matches an equivalent one. The equivalent letters are defined as follows [[Cornish-Bowden, 1984](https://www.ncbi.nlm.nih.gov/pmc/articles/PMC341218/)]:

```
    SYMBOL | DESCRIPTION                   | BASES   | COMPLEMENT
    -------+-------------------------------+---------+-----------
       A   | Adenine                       | A       |  T
       C   | Cytosine                      |   C     |  G
       G   | Guanine                       |     G   |  C
       T   | Thymine                       |       T |  A
       W   | Weak                          | A     T |  W
       S   | Strong                        |   C G   |  S
       M   | aMino                         | A C     |  K
       K   | Keto                          |     G T |  M
       R   | puRine                        | A   G   |  Y
       Y   | pYrimidine                    |   C   T |  R
       B   | not A (B comes after A)       |   C G T |  V
       D   | not C (D comes after C)       | A   G T |  H
       H   | not G (H comes after G)       | A C   T |  D
       V   | not T (V comes after T and U) | A C G   |  B
       N   | aNy base (not a gap)          | A C G T |  N
    -------+-------------------------------+---------+----------
```

If the reference allele is not valid, the `normalize_variant` function tries to find a reference match with one of the following variant transformations:

* **swap** the reference and alternate alleles - *sometimes it is not clear which one is the reference and which one is the alternate allele*.
* **flip** the alleles letters (use the **complement** letters) - *sometimes the alleles refers to the other DNA strand*.
* **swap** and **flip**.

Note that the *swap* and *flip* processes can lead to false positive cases, especially when considering *Single Nucleotide Polymorphisms* (SNPs). The return code of the `normalize_variant` function can be used to discriminate or discard variants that are not consistent.

If the variant doesn't match the genome reference, then the original variant is returned with an error code.

If both alleles have length 1, the normalization is complete and the variant is returned.
Otherwise, a custom implementation of the [vt normalization](https://genome.sph.umich.edu/wiki/Vt#Normalization) algorithm is applied:

```
while break, do
    if any of the alelles is empty and the position is greater than zero, then
        extend both alleles one letter to the left using the nucleotide in
        the corresponding genome reference position;
    else
        if both alleles end with the same letter and they have length 2 or more, then
            truncate the rightmost letter of each allele;
        else
            break (exit the while loop);

while both alleles start with the same letter and have length 2 or more, do
    truncate leftmost letter of each allele;
```

The genome reference binary file can be obtained from a FASTA file using the `resources/tools/fastabin.sh` script.
This script extracts the first 25 sequences for chromosomes `1` to `22`, `X`, `Y` and `MT`.

#### Normalized VariantKey

This library provides the `normalized_variantkey` function that returns the VariantKey of the normalized variant.
This function should be used instead of `variantkey` if the input variant is not normalized.


<a name="vkformat"></a>
## VariantKey Format

For a given reference genome the VariantKey format encodes a *Human Genetic Variant* (`CHROM`, `POS`, `REF` and `ALT`) as 64 bit unsigned integer number (8 bytes or 16 hexadecimal symbols).
If the variant has not more than 11 bases between `REF` and `ALT`, the correspondent VariantKey can be directly reversed to get back the individual `CHROM`, `POS`, `REF` and `ALT` components.
If the variant has more than 11 bases, or non-base nucleotide letters are contained in `REF` or `ALT`, the VariantKey can be fully reversed with the support of a binary lookup table.


The VariantKey is composed of 3 sections arranged in 64 bit:


```
         0   4 5                             32 33                              63
         |   | |                              | |                                |
         01234 567 89012345 67890123 45678901 2 3456789 01234567 89012345 67890123
5 bit CHROM >| |<         28 bit POS         >| |<        31 bit REF+ALT        >|
```

Example of VariantKey encoding:

```
                   | CHROM | POS                          | REF | ALT                           |
-------------------+-------+------------------------------+-----+-------------------------------+
       Raw variant | chr19 | 29238770                     | TC  | TG                            |
Normalized variant | 19    | 29238771                     | C   | G                             |
-------------------+-------+------------------------------+-----+-------------------------------+
    VariantKey bin | 10011 | 0001101111100010010111110011 | 0001 0001 01 10 0000000000000000000 |
-------------------+-------+------------------------------+-------------------------------------+
    VariantKey hex | 98DF12F988B00000                                                           |
    VariantKey dec | 11015544076520914944                                                       |
-------------------+----------------------------------------------------------------------------+
```


* **`CHROM`** : 5 bit to represent the chromosome.

    ```
         0   4
         |   |
         11111000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
         |   |
         MSB LSB

         CHROM binary mask (F800000000000000 hex = 17870283321406128128 dec)

         Example: 'chr19' str = 19 dec = 10011 bin
    ```

    The chromosome is encoded as unsigned integer number: 1 to 22, X=23, Y=24, MT=25, NA=0.  
    This section is 5 bit long, so it can store up to 2<sup>5</sup>=32 symbols, enough to contain the required 25 canonical chromosome symbols + NA.  
    The largest value is: 25 dec = 19 hex = 11001 bin.  
    Values from 26 to 31 are currently reserved. They can be used to indicate 6 alternative modes to interpret the remaining 59 bit. For instance, one of these values can be used to indicate the encoding of variants that occurs in non-canonical contigs.  


* **`POS`** : 28 bit for the reference position (`POS`), with the first nucleotide having position 0.

    ```
         0    5                              32
         |    |                              |
         00000111 11111111 11111111 11111111 10000000 00000000 00000000 00000000
              |                              |
              MSB                            LSB

         POS binary mask (7FFFFFF80000000 hex = 576460750155939840 dec)

         Example: 29238771 dec = 0001101111100010010111110011 bin
    ```

    This section is 28 bit long, so it can store up to 2<sup>28</sup>=268,435,456 symbols, enough to contain the maximum position found on the largest human chromosome.


* **`REF+ALT`** : 31 bit for the encoding of the `REF` and `ALT` strings.

    ```
         0                                    33                               63
         |                                    |                                |
         00000000 00000000 00000000 00000000 01111111 11111111 11111111 11111111
                                              |                                |
                                              MSB                              LSB

         REF+ALT binary mask (7FFFFFFF hex = 2147483647 dec)
    ```

    This section allows two different encodings:

    * **Non-reversible encoding**

        If the total number of nucleotides between `REF` and `ALT` is more than 11, or if any of the alleles contains nucleotide letters other than base `A`, `C`, `G` and `T`, then the LSB (least significant bit) is set to 1 and the remaining 30 bit are filled with a hash value of the `REF` and `ALT` strings.  
        The hash value is calculated using a custom fast non-cryptographic algorithm based on [MurmurHash3](https://github.com/aappleby/smhasher/wiki/MurmurHash3).  
        A lookup table is required to reverse the `REF` and `ALT` values.  
        In the normalized dbSNP VCF file GRCh37.p13.b150 there are only 0.365% (1229769 / 337162128) variants that requires this encoding. Amongst those, the maximum number of variants that share the same chromosome and position is 15. With 30 bit the probability of hash collision is approximately 10<sup>-7</sup> for 15 elements, 10<sup>-6</sup> for 46 and 10<sup>-5</sup> for 146.
        The size of the non-reversible lookup table for GRCh37.p13.b150 is only 45.7MB.

    * **Reversible encoding**

        If the total number of nucleotides between `REF` and `ALT` is 11 or less, and they only contain base letters `A`, `C`, `G` and `T`, then the LSB is set to 0 and the remaining 30 bit are used as follows:  
        * bit 1-4 indicates the number of bases in `REF` - the capacity of this section is 2<sup>4</sup>=16; the maximum expected value is 10 dec = 1010 bin;
        * bit 5-8 indicates the number of bases in `ALT` - the capacity of this section is 2<sup>4</sup>=16; the maximum expected value is 10 dec = 1010 bin;
        * the following 11 groups of 2 bit are used to represent `REF` bases followed by `ALT`, with the following encoding:
            * `A` = 0 dec = 00 bin;
            * `C` = 1 dec = 01 bin;
            * `G` = 2 dec = 10 bin;
            * `T` = 3 dec = 11 bin.

        Examples:

        ```
            REF     ALT        REF+ALT BINARY ENCODING
            A       G          0001 0001 00 10 00 00 00 00 00 00 00 00 00 0
            GGG     GA         0011 0010 10 10 10 10 00 00 00 00 00 00 00 0
            ACGT    CGTACGT    0100 0111 00 01 10 11 01 10 11 00 01 10 11 0
                               |                                          |
                               33 (MSB)                                   63 (LSB)
        ```

        The reversible encoding covers 99.635% of the variants in the normalized dbSNP VCF file GRCh37.p13.b150.

<a name="vkproperties"></a>
### VariantKey Properties

* It can be encoded and decoded on-the-fly.
* Sorting by VariantKey is equivalent of sorting by `CHROM` and `POS`.
* The 64 bit VariantKey can be exported as a 16 character hexadecimal string.
* Sorting the hexadecimal representation of VariantKey in alphabetical order is equivalent of sorting the VariantKey numerically.
* Each VariantKey code is unique for a given reference genome.
* The direct comparisons of two VariantKeys makes sense only if they both refer to the same genome reference.
* Comparing two variants by VariantKey only requires comparing two 64 bit numbers, a very well optimized operation in current computer architectures. In contrast, comparing two normalized variants in VCF format requires comparing one numbers and three strings.
* VariantKey can be used as a main database key to index data by "variant". This simplifies common searching, merging and filtering operations.
* All types of database joins between two data sets (inner, left, right and full) can be easily performed using the VariantKey as index.
* When `CHROM`, `REF` and `ALT` are the only strings in a table, replacing them with VariantKey allows to work with numeric only tables with obvious advantages. This also allows to represent the data in a compact binary format where each column uses a fixed number of bit, with the ability to perform a quick binary search on the first sorted column.


<a name="vkinput"></a>
## VariantKey Input values

* **`CHROM`** - *chromosome*     : Identifier from the reference genome, no white-space permitted.
* **`POS`**   - *position*       : The reference position, with the first nucleotide having position 0.
* **`REF`**   - *reference allele* :
    String containing a sequence of [nucleotide letters](https://en.wikipedia.org/wiki/Nucleic_acid_notation).
    The value in the `POS` field refers to the position of the first nucleotide in the string.
* **`ALT`**   - *alternate non-reference allele* : 
    String containing a sequence of [nucleotide letters](https://en.wikipedia.org/wiki/Nucleic_acid_notation).

----------

<a name="regionkey"></a>
## RegionKey

*RegionKey* encodes a human genomic region (defined as the set of *chromosome*, *start position*, *end position* and *strand direction*) in a 64 bit unsigned integer number.

RegionKey represents a region as a single entity, and provides the same properties listed in [VariantKey Properties](#vkproperties).

The encoding of the first 33 bit (CROM, STARTPOS) is the same as in VariantKey.

The RegionKey is composed of 4 sections arranged in 64 bit:


```
         0   4 5                             32 33                           60    63
         |   | |                              | |                             |    |
         01234 567 89012345 67890123 45678901 2 3456789 01234567 89012345 67890 12 3
5 bit CHROM >| |<      28 bit START POS      >| |<      28 bit END POS       >| ||
                                                                                STRAND
```


Example of RegionKey encoding:

```
                  | CHROM | STARTPOS                     | ENDPOS                       | STRAND |
------------------+-------+------------------------------+------------------------------+--------+
      Raw variant | chr19 | 29238771                     | 29239026                     | +1     |
Normalized region | 19    | 29238771                     | 29239026                     | +1     |
------------------+-------+------------------------------+------------------------------+--------+
    RegionKey bin | 10011 | 0001101111100010010111110011 | 0001101111100010011011110010 | 01 0   |
------------------+-------+------------------------------+---------------------------------------+
    RegionKey hex | 98DF12F98DF13792                                                             |
    RegionKey dec | 11015544076609075090                                                         |
------------------+------------------------------------------------------------------------------+
```

* **`CHROM`**   : 5 bit to represent the chromosome.  
  An identifier from the reference genome. It only has 26 valid values: autosomes from 1 to 22, the sex chromosomes X=23 and Y=24, mitochondria MT=25 and a symbol NA=0 to indicate an invalid value.

    ```
        0   4
        |   |
        11111000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
        |   |
        MSB LSB

        CHROM binary mask (F800000000000000 hex = 17870283321406128128 dec)

        Example: 'chr19' str = 19 dec = 10011 bin
    ```

    The chromosome is encoded as in VariantKey.

* **`STARTPOS`** : 28 bit for the region START position.  
  The region start position in the chromosome, with the first base having position 0. The largest expected value is less than 250 million to represent the last base pair in Chromosome 1.

    ```
        0    5                              32                                63
        |    |                              |                                 |
        00000111 11111111 11111111 11111111 10000000 00000000 00000000 00000000
             |                              |
             MSB                            LSB

        STARTPOS binary mask (7FFFFFF80000000 hex = 576460750155939840 dec)

        Example: 29238771 dec = 0001101111100010010111110011 bin
    ```

    This section is encoded as in VariantKey POS.

* **`ENDPOS`** : 28 bit for the region END position.  
  The region end position in the chromosome. The end position is equivalent to (STARTPOS + REGION_LENGTH), such that the base having position ENDPOS is not included in the region.

```
        0                                    33                            60 63
        |                                    |                             |  |
        00000000 00000000 00000000 00000000 01111111 11111111 11111111 11111000
                                             |                             |
                                             MSB                           LSB

        ENDPOS binary mask (7FFFFFF8 hex = 2147483640 dec)

        Example: 29239026 dec = 0001101111100010011011110010 bin
```
    The end position is equivalent to (STARTPOS + REGION_LENGTH).

* **`STRAND`** : 2 bit to encode the strand direction.  
  (optional) The direction of the DNA strand. This is useful when encoding genic regions.

    ```
        0                                                                 61  62
        |                                                                   ||
        00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000110
                                                                            ||
                                                                         MSB  LSB
    ```

    The strand direction is encoded as:

    ```
    -1 : 2 dec = "10" bin = reverse (minus) strand direction
     0 : 0 dec = "00" bin = unknown or not applicable strand direction
    +1 : 1 dec = "01" bin = forward (plus) strand direction
    ```

* The last bit of RegionKey is reserved.

This software library provides several functions to operate with *RegionKey* and interact with *VariantKey*.


<a name="rkproperties"></a>
### RegionKey Properties

* It is compatible with VariantKey.
* It can be encoded and decoded on-the-fly.
* Sorting by RegionKey is equivalent of sorting by CHROM and STARTPOS.
* The 64 bit RegionKey can be exported as a single 16 character hexadecimal string.
* Sorting the hexadecimal representation of RegionKey in alphabetical order is equivalent of sorting the RegionKey numerically.
* RegionKey can be used as a main database key to index data by "region". This simplifies common searching, merging and filtering operations.

----------

<a name="esid"></a>
## Encoding String IDs

This library contains extra functions to encode some string IDs to 64 bit unsigned integers:

* The `encode_string_id` function encodes up to 10 ASCII characters (from '!' to 'z') of a string into a 64 bit unsigned integer. The encoded value can be reversed into a "normalized" version of the original 10 character string using the `decode_string_id` function. The decoded string only supports uppercase characters.

* The `encode_string_num_id` function encodes a string composed of a character section, a separator character and a numerical section into a 64 bit unsigned integer. For example: "`ABCDE:0001234`". This function encodes up to 5 characters in uppercase, a number up to 2<sup>27</sup>, and up to 7 zero padding digits in a 64 bit unsigned integer. The encoded value can be reversed into a "normalized" version of the original 10 character string using the `decode_string_id` function.

* The `hash_string_id` function creates a 64 bit unsigned integer hash of the input string.

----------

<a name="binaryfiles"></a>
## Binary files for lookup tables

A direct application of the VariantKey representation is the ability to create lookup tables as simple binary files.  
The binary lookup-table files are natively supported by the variantkey library and can be generated using the scripts in `resources/tools/`.  
NOTE: The `vkhexbin.sh` script requires [bcftools](https://github.com/samtools/bcftools/tree/develop) with variantkey support.
The `vcfnorm.sh` script requires the [vt](https://github.com/atks/vt) tool.

Prebuilt binary files can be downloaded from:
https://sourceforge.net/projects/variantkey/files/

* **`fasta.bin`**
    Binary version of the reference genome sequence FASTA file.  
    It only contains the first 25 sequences for chromosomes 1 to 22, X, Y and MT.  
    This binary file can be generated by the [fastabin.sh](https://github.com/tecnickcom/variantkey/blob/main/resources/tools/fastabin.sh) script from a genome reference FASTA file.
    
* **`rsvk.bin`**
    Lookup table to retrieve VariantKey from rsID.  
    This binary file can be generated by the [rsvk.sh](https://github.com/tecnickcom/variantkey/blob/main/resources/tools/rsvk.sh) script from a normalized TSV file.
    The VCF file can be normalized using the [vcfnorm.sh](https://github.com/tecnickcom/variantkey/blob/main/resources/tools/vcfnorm.sh) script.  
    This can also be in *Apache Arrow File* format with a single *RecordBatch*, or *Feather* format. The first column must contain the rsID sorted in ascending order.
    
* **`vkrs.bin`**
    Lookup table to retrieve rsID from VariantKey.  
    This binary file can be generated by the [vkrs.sh](https://github.com/tecnickcom/variantkey/blob/main/resources/tools/vkrs.sh) script from a normalized TSV file.
    The VCF file can be normalized using the [vcfnorm.sh](https://github.com/tecnickcom/variantkey/blob/main/resources/tools/vcfnorm.sh) script.
    This can also be in *Apache Arrow File* format with a single *RecordBatch*, or *Feather* format. The first column must contain the VariantKey sorted in ascending order.
    
* **`nrvk.bin`**
    Lookup table to retrieve the original `REF` and `ALT` string for the non-reversible VariantKey.  
    This binary file can be generated by the [nrvk.sh](https://github.com/tecnickcom/variantkey/blob/main/resources/tools/nrvk.sh) script from a TSV file with the following format:

    ```
    [16 BYTE VARIANTKEY HEX]\t[REF STRING]\t[ALT STRING]\n...

    for example:

    b800c35bbcece603	AAAAAAAAGG	AG
    1800c351f61f65d3	A	AAGAAAGAAAG
    ```

----------

<a name="annotations"></a>
## Annotation lookup tables

Variant annotation sets, such as pathogenicity predictions, allele frequencies or
model scores, are normally distributed as a bgzip-compressed TSV with a tabix
index, keyed on the four `CHROM`, `POS`, `REF` and `ALT` columns.

The `vkbin` tool converts any such file into a VariantKey-indexed
[BINSRC1](#binaryfiles) binary lookup table, so that the annotation can be
memory mapped, searched with `binsearch` and joined with other data on a single
64 bit integer.

<a name="whyconvert"></a>
### Why convert an annotation file

A tabix TSV is a compact archive and reads well in genomic order, but it is not
very efficient for repeated point lookups and joins on the variant.

A single `uint64` replaces the four source columns as the join key. Joining two
annotation sets, or an annotation set and a cohort, no longer requires matching
a string chromosome that may or may not carry a `chr` prefix, then a position,
then two allele strings. A columnar engine such as DuckDB, Spark or BigQuery can join
on that key directly, and none of them reads a tabix index.

Point lookups need neither decompression nor parsing. A tabix query seeks to the
start of the index bin holding the target, then inflates and parses forward to
reach it. A BINSRC1 file is fixed-width records in memory mapped pages, so a
lookup is a binary search over integers: `O(log n)` comparisons, no allocation,
no text.

Over 10 million rows of the AVI score file, with both files resident in the page
cache, a point lookup takes 99 ns against the 2.10 ms of a tabix query using
the index `tabix` builds by default. A CSI index with 64 bp bins brings tabix to
77 us, which is its floor: reaching one record means inflating the 64 KiB block
that holds it. Tabix bins are measured in genomic coordinates, so on saturation
data at 2.8 records per base pair a default 16 kb bin spans about 46,000 records.
Once the table is larger than RAM, cost is set by page cache residency rather
than by either search and the two move closer together. See
[BENCHMARKS.md](BENCHMARKS.md) for the method and the full measurements.

Storage is smaller uncompressed and at every compression level short of `xz`.
Measured over ten million rows of the AlphaGenome AVI score file and
extrapolated to its full 9.06e9 rows:

| Representation       | Uncompressed | gzip -6  | zstd -3  | zstd -12 | xz -6   |
|----------------------|--------------|----------|----------|----------|---------|
| BINSRC1, 16 bytes    | 145.0 GB     | 68.8 GB  | 58.0 GB  | 53.2 GB  | 40.4 GB |
| source TSV           | 298.1 GB     | 87.6 GB  | 78.8 GB  | 62.4 GB  | 40.2 GB |

That is 2.06x smaller uncompressed and 15% to 26% smaller at the usual
compression levels. The two forms converge at `xz`, which finds the same
structure in both. Most of the gain comes from the key column: over a dense
variant set the sorted VariantKeys are close to an arithmetic sequence, so it
compresses far better than the values it indexes. Under `gzip -6` it reaches 28%
of its size against the 62% and 73% of the two value columns, and under `xz -6`
it reaches 6%.

Values are stored exactly. Each value column is a decimal scaled to an integer,
so a score printed with five decimals is read back with five decimals, with no
rounding through a `float`.

The trade is disk space. A tabix file is queried in place while still compressed,
whereas a BINSRC1 file must be uncompressed to be memory mapped: at the
extrapolated AVI sizes, 145.0 GB against the 88.5 GB of the distributed file.

<a name="vkbintool"></a>
### The vkbin tool

The code inside the `c/vkbin` folder generates the `vkbin` command line tool.
It reads a TSV on standard input and writes a BINSRC1 file:

```
Usage: vkbin -o FILE [-s N] COLSPEC...

  -o FILE  Output file. Required.
  -s N     Number of leading header lines to skip. Default 1.
  -h       This help.
```

The first four input columns must be `CHROM`, `POS`, `REF` and `ALT`, where
`POS` is 1-based as in VCF. Each `COLSPEC` describes one further column:

```
WIDTH:DECIMALS[:OFFSET][:NA]
```

* **`WIDTH`**    - *stored size in bytes*: 1, 2, 4 or 8.
* **`DECIMALS`** - *decimal places to preserve*: the value is multiplied by
    `10^DECIMALS` and rounded to an integer, half away from zero.
* **`OFFSET`**   - *decimal added before scaling*, to shift a signed range into
    the unsigned one. Default 0.
* **`NA`**       - *store the largest value of `WIDTH` for a missing field*,
    instead of stopping. A real value that reaches that largest value is then
    rejected, so the two cannot be confused at query time.

`OFFSET` and `NA` follow `DECIMALS` in any order and each may appear once.

Value fields are decimal numbers, in plain or exponential notation. With `NA` a
field is missing when it is empty or is `.`, `NA`, `N/A`, `NaN` or `null`, in
any case.

Input columns beyond the last `COLSPEC` are ignored, so one invocation keeps
working when the source file gains columns.

Decompression is left to the caller, so the tool has no dependency beyond the C
standard library:

```sh
bgzip -dc annotations.tsv.gz | vkbin -o annotations.bin 4:5:2.0 4:5:0
```

The [annotbin.sh](https://github.com/tecnickcom/variantkey/blob/main/resources/tools/annotbin.sh)
script in `resources/tools/` wraps that pipeline, selecting the decompressor
from the file name:

```sh
ANNOT_INPUT_FILE=annotations.tsv.gz \
ANNOT_OUTPUT_FILE=annotations.bin \
ANNOT_COLUMNS="4:5:2.0 4:5:0" \
./annotbin.sh
```

The input must already be sorted by VariantKey, as a file sorted by chromosome
and position is. The tool stops with an error on an unsorted input, because an
unsorted table cannot be binary searched and would fail silently at query time,
and on a value that does not fit its declared width, rather than truncating it.

<a name="aviexample"></a>
### Worked example: AlphaGenome AVI scores

Google DeepMind's AlphaGenome Atlas publishes an
[AVI score](https://deepmind.google.com/science/alphagenome/downloads) for every
possible single nucleotide variant in the human genome: 9.06e9 rows, 88.5 GB as
distributed. The download is `avi_scores_snvs_tabix.zip`, holding the bgzip TSV
`alphagenome_variant_impact_score_snvs.tsv.gz`. Its first rows are:

```
#CHROM  POS    REF  ALT  raw_score  PHRED
chr1    10001  T    A    -0.03868   1.06466
chr1    10001  T    C    -0.032     1.3114
chr1    10001  T    G    -0.0372    1.11839
```

Both scores carry five decimals. `raw_score` is signed, spanning roughly -1.3 to
+4.6, so an offset of `2.0` shifts it into the unsigned range. `PHRED` is already
non-negative:

```sh
bgzip -dc alphagenome_variant_impact_score_snvs.tsv.gz \
  | vkbin -o avi.bin 4:5:2.0 4:5:0
```

The first row above is stored as:

| Field       | Source          | Stored                                    |
|-------------|-----------------|-------------------------------------------|
| VariantKey  | chr1:10001 T>A  | `0800138808e00000`                        |
| `raw_score` | -0.03868        | `196132`, that is `(-0.03868 + 2.0) * 1e5` |
| `PHRED`     | 1.06466         | `106466`, that is `1.06466 * 1e5`          |

<a name="readoutput"></a>
### Reading the output

Output files are ordinary BINSRC1 files, read by the library with no extra
support. Column 0 is the VariantKey, and the value columns follow in the order
they were given on the command line:

```c
#include "variantkey/binsearch.h"
#include "variantkey/variantkey.h"

mmfile_t mf = {0};
mmap_binfile("avi.bin", &mf); // the header supplies nrows, ncols and index[]

const uint64_t *keys   = get_src_offset_uint64_t(mf.src, mf.index[0]);
const uint32_t *raws   = get_src_offset_uint32_t(mf.src, mf.index[1]);
const uint32_t *phreds = get_src_offset_uint32_t(mf.src, mf.index[2]);

// The VariantKey POS is 0-based, so it is one less than the POS in the file.
uint64_t vk[2] =
{
    variantkey("chr1", 4, 10000, "T", 1, "A", 1),
    variantkey("chr1", 4, 10000, "T", 1, "C", 1),
};
uint64_t pos[2] = {0};

col_find_many_le_uint64_t(keys, 0, mf.nrows, vk, pos, 2, mf.prefetch);

uint64_t k = 0;
for (k = 0; k < 2; k++)
{
    if (pos[k] < mf.nrows) // mf.nrows is reported when the key is absent
    {
        uint32_t raw_q = raws[pos[k]];     // 196132 for the first key
        uint32_t phred_q = phreds[pos[k]]; // 106466 for the first key
    }
}
```

Search a batch with `col_find_many_le_uint64_t` rather than calling
`col_find_first_le_uint64_t` once per variant. Both report the item number, or
`nrows` when the key is absent, but `find_many` advances the searches in lockstep
so that their cache misses overlap, which is worth 2 to 3 times per value over
the AVI tables. On a file too large to cache, `BINSEARCH_PREFETCH_AUTO` also asks
for the pages of a batch before reading them. `mmap_binfile` sets `mf.prefetch`
to that value.

Use `col_find_first_le_uint64_t` when there is a single key:

```c
uint64_t first = 0;
uint64_t last = mf.nrows;
uint64_t key = variantkey("chr1", 4, 10000, "T", 1, "A", 1);
uint64_t i = col_find_first_le_uint64_t(keys, &first, &last, key);

if (i < mf.nrows)
{
    uint32_t raw_q = raws[i];     // 196132
    uint32_t phred_q = phreds[i]; // 106466
}

munmap_binfile(&mf);
```

Print a stored value by formatting the integer with its decimals. Converting it
to a `float` first reintroduces the representation error that the fixed-point
encoding avoids.

Column 0 holds an unmodified VariantKey, so the same file can be joined directly
against any other VariantKey-keyed data, and any row can be turned back into
`CHROM`, `POS`, `REF` and `ALT` with `reverse_variantkey`.

<a name="colwidths"></a>
### Choosing column widths

A value needs `ceil(log2(range * 10^decimals))` bits, and BINSRC1 stores columns
of 1, 2, 4 or 8 bytes, so the requirement rounds up to the next of those sizes.
For the AVI scores at five decimals:

| Column      | Observed range   | Scaled range | Bits | Width |
|-------------|------------------|--------------|------|-------|
| `raw_score` | -1.269 to +4.557 | 5.83e5       | 20   | 4 B   |
| `PHRED`     | 0.0 to +82.9945  | 8.30e6       | 23   | 4 B   |

Prefer the wider column when the choice is close: the observed range of a source
file is a lower bound on its true range, a value that does not fit stops the
conversion, and spare capacity costs only bytes that largely compress away. Both
AVI columns need 20 and 23 bits and are given 4 bytes.

----------

<a name="clib"></a>
## C Library

* [C source code documentation](https://tecnickcom.github.io/variantkey/c/index.html)
* [C Usage Examples](c/test/test_example.c)

The reference implementation of this library is written in header-only C programming language in a way that is also compatible with C++.

This project includes a Makefile that allows you to test and build the project in a Linux-compatible system with simple commands.  
All the artifacts and reports produced using this Makefile are stored in the *target* folder.  

* To see all available options: `make help`
* To build everything: `make all`

### Example command-line tool

The code inside the `c/vk` folder generates the `vk` command line tool.  
It takes the pre-normalized positional arguments `CHROM`, `POS`, `REF` and `ALT`, and returns the VariantKey in hexadecimal representation.

### Annotation table builder

`vkbin`, in the `c/vkbin` folder, converts an annotation TSV into a VariantKey-indexed binary lookup table.
See [Annotation lookup tables](#annotations).


<a name="golib"></a>
## Go Library (golang)

* [Go source code documentation](https://tecnickcom.github.io/variantkey/go/index.html)
* [Go Usage Examples](go/src/example_variantkey_test.go)

The Go wrapper is located in the `go` directory.  
Use the "`make go`" command to test it and generate reports.


<a name="pythonlib"></a>
## Python Module

* [Python source code documentation](https://tecnickcom.github.io/variantkey/python/variantkey.html)
* [Python Usage Examples](python/test/example.py)

The Python module is located in the `python` directory.
Use the "`make python`" command to test it and generate reports.


<a name="pythonclass"></a>
## Python Vectorized Class

* [Python vectorized class source code documentation](https://tecnickcom.github.io/variantkey/python-class/pyvariantkey.variantkey.html)
* [Python Usage Examples](python-class/test/example.py)

The Python class module wraps the low-level Python library and is located in the `python-class` directory.
All methods of this class are vectorized, so they also accept lists or numpy arrays as input.
Use the "`make python-class`" command to test it and generate reports.


<a name="rlib"></a>
## R Module

* [R source code documentation](https://tecnickcom.github.io/variantkey/r/index.html)
* [R Usage Examples](r/example/example.R)

The R module is located in the `r` directory.
Use the "`make r`" command to test it and generate reports.

In R the VariantKey is represented with a custom "uint64" class because there is no native support for unsigned 64 bit integers in R.


<a name="jslib"></a>
## Javascript library (limited support)

Use the "`make javascript`" command to test and minify the Javascript implementation.
