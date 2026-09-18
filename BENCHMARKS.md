# Benchmarks

Measurements of the annotation lookup tables built by
[vkbin](README.md#annotations).

Every figure below is machine specific, and every ratio between two of them
moves with the size of the data and with how much of it the page cache holds.
Each section states both.

-----------------------------------------------------------------

## Environment

12th Gen Intel i7-1260P, 30 GiB RAM, SK hynix PC801 NVMe, ext4. Debian 13,
Linux 6.12.107, gcc 14.2.0, `-O3 -std=c17`. htslib 1.24. VariantKey 5.9.1.

-----------------------------------------------------------------

## Data

The first 10,000,000 data rows of the AlphaGenome AVI score file
`alphagenome_variant_impact_score_snvs.tsv.gz`, `chr1:10001` to `chr1:3536843`,
with `raw_score` and `PHRED` at five decimals each. Over that span the file
carries 2.8 records per base pair.

| Representation | Bytes |
|----------------|--------------|
| TSV | 329,034,886 |
| TSV, bgzip | 94,111,732 |
| BINSRC1, `4:5:2.0 4:5:0` | 160,000,048 |

-----------------------------------------------------------------

## Point lookup against tabix

5,000 random variants. Both paths run in one process over the same query set and
return the same scaled `raw_score`, so the results are compared before timing.
Each figure is the mean of three rounds after a warmup pass. The data file and
its index are resident in the page cache.

| Lookup | Index | Bin | ns per lookup |
|--------|-------|-----|---------------|
| `tbx_itr_queryi` | TBI, as `tabix` builds by default | 16 kb | 2,100,000 |
| `tbx_itr_queryi` | CSI, `-m 14` | 16 kb | 2,140,000 |
| `tbx_itr_queryi` | CSI, `-m 10` | 1 kb | 191,000 |
| `tbx_itr_queryi` | CSI, `-m 6` | 64 bp | 77,000 |
| `col_find_first_le_uint64_t` | none | | 99 |

A tabix query seeks to the start of the bin holding the target and then inflates
and parses forward to reach it, so its cost is the bin size times the record
density. The index bins are measured in genomic coordinates. At 2.8 records per
base pair a 16 kb bin spans about 46,000 records and 1.5 MB of text, and the
iterator walks all of it to answer one variant. Shrinking the bin 16 times cuts
the time 11 times, and again by 16 cuts it a further 2.5 times.

77 us is the floor. Reaching one record means inflating the 64 KiB bgzf block
that holds it, at about 1 GB/s.

This density is a property of saturation data, where every covered position
carries all three alternate alleles: 3 records per base pair, less the positions
the file leaves out. On a sparse annotation set, where a 16 kb window holds a
handful of records, the tabix figures are much lower.

-----------------------------------------------------------------

## Point lookup against table size

`col_find_first_le_uint64_t` over the memory mapped VariantKey column of three
BINSRC1 files built from the head of the AVI score file. 200,000 random keys
drawn from the table itself, so every search hits. The first pass runs once
after `MADV_DONTNEED` and `POSIX_FADV_DONTNEED`, and moves by about 20% between
runs; the resident cost is the fastest of three further rounds.

| Rows | Key column | First pass | Resident |
|------|------------|------------|----------|
| 1,000,000 | 8 MB | 150 ns | 40 ns |
| 10,000,000 | 80 MB | 1,490 ns | 146 ns |
| 100,000,000 | 800 MB | 12,500 ns | 288 ns |

The first pass is the page faults of the memory mapping, and it grows with the
file. The resident cost grows too, though far more slowly: `log2(n)` adds about
3 comparisons per decade, and once the key column is past the last level of
cache each of them is a memory access.

The query set decides how much of that shows. Over the 5,000 variants of the
tabix comparison the same 10,000,000 row table answers in 99 ns, because the
upper levels of the search stay in cache across queries.

-----------------------------------------------------------------

## Larger than memory

The figures above are for files that fit in RAM. They do not carry over to a
table the page cache cannot hold, where the number that matters is residency
rather than either search.

The two formats then fail differently. A BINSRC1 lookup pays in page faults,
about `log2(file size / page cache available to it)` device reads per search,
and that cost falls to zero on a machine that can hold the working set. A tabix
lookup pays in decompression, and no amount of memory removes it.

Use `col_find_many_le_uint64_t` for anything past a single key. The searches of
a batch advance in lockstep so that their cache misses overlap, which over the
three resident tables above is worth 1.7 times per value at 1,000,000 rows, 2.0
at 10,000,000 and 2.5 to 3.4 at 100,000,000.

`BINSEARCH_PREFETCH_AUTO` also asks for the pages of a batch before any of them
is read. That is aimed at a table the page cache cannot hold, which none of
these is: on the 800 MB table with the cache dropped it cost 29,235 ns per value
against the 7,003 ns of `BINSEARCH_PREFETCH_NEVER`.

-----------------------------------------------------------------

## Conversion

`vkbin -o avi.bin 4:5:2.0 4:5:0` over the 10,000,000 row TSV, input resident,
three runs: 0.692 s, 0.699 s, 0.711 s. About 14.3 million rows/s on one core.

-----------------------------------------------------------------

## Reproducing

The input is the AVI SNV scores archive from the
[AlphaGenome downloads page](https://deepmind.google.com/science/alphagenome/downloads).
Its TSV member is stored rather than deflated, so the bgzip stream reads
straight out of the archive when the member is named:

```sh
unzip -p avi_scores_snvs_tabix.zip alphagenome_variant_impact_score_snvs.tsv.gz \
  | gzip -dc | head -n 10000001 > avi10m.tsv
vkbin -o avi.bin 4:5:2.0 4:5:0 < avi10m.tsv
```

Neither timed program is part of the test suite, the tabix one because it needs
htslib. For that comparison, `bgzip` the TSV, index it with
`tabix -s1 -b2 -e2`, and build the CSI variants with `tabix -C -m INT`. Then
time `tbx_itr_queryi` followed by a field parse against `variantkey()` followed
by `col_find_first_le_uint64_t`, in one process, over one query set, after a
warmup pass. Compare the values both paths return before recording any time.

For the table size figures, `mmap_binfile` the output of `vkbin`, draw the
queries from the key column, and time `col_find_first_le_uint64_t` once after
`madvise(MADV_DONTNEED)` and `posix_fadvise(POSIX_FADV_DONTNEED)` and again with
the file resident.
