
# Rtskit

[![R-CMD-check](https://github.com/RGenomicsETL/Rtskit/actions/workflows/R-CMD-check.yaml/badge.svg)](https://github.com/RGenomicsETL/Rtskit/actions/workflows/R-CMD-check.yaml)
[![pkgdown](https://github.com/RGenomicsETL/Rtskit/actions/workflows/pkgdown.yaml/badge.svg)](https://rgenomicsetl.github.io/Rtskit/)
[![R-universe](https://rgenomicsetl.r-universe.dev/badges/Rtskit)](https://rgenomicsetl.r-universe.dev/Rtskit)

`Rtskit` is a native R interface to succinct tree sequences through the
stable [`tskit` C API](https://tskit.dev/tskit/docs/stable/c-api.html).
It does not use Python, `reticulate`, Rust, Java, or a system tskit
shared library.

The initial interface keeps each `tsk_treeseq_t` under one native
external pointer and exposes deliberate materialization operations for
summaries, sample nodes, node, edge, population, and individual tables,
opaque metadata bytes, tree intervals, and source-population ancestry
intervals.

Install the development build from R-universe:

``` r
install.packages(
  "Rtskit",
  repos = c(
    RGenomicsETL = "https://rgenomicsetl.r-universe.dev",
    CRAN = "https://cloud.r-project.org"
  )
)
```

``` r
library(Rtskit)

trees <- tskit_example()
trees
```

    ## <Rtskit::TreeSequence>
    ##   source: <memory>
    ##   sequence length: 10
    ##   trees: 2
    ##   samples: 2

``` r
tskit_nodes(trees)
```

    ##   id flags time population individual
    ## 1  0     1    0         -1          0
    ## 2  1     1    0         -1          0
    ## 3  2     0    1          0         -1
    ## 4  3     0    1          1         -1

``` r
tskit_edges(trees)
```

    ##   id left right parent child
    ## 1  0    0     5      2     0
    ## 2  1    0     5      2     1
    ## 3  2    5    10      3     0
    ## 4  3    5    10      3     1

``` r
tskit_trees(trees)
```

    ##   index left right roots edges
    ## 1     0    0     5     1     2
    ## 2     1    5    10     1     2

``` r
tskit_ancestry_intervals(
  trees,
  c("panel:source_A" = 0L, "panel:source_B" = 1L)
)
```

    ##   sample left right         source source_population
    ## 1      0    0     5 panel:source_A                 0
    ## 2      0    5    10 panel:source_B                 1
    ## 3      1    0     5 panel:source_A                 0
    ## 4      1    5    10 panel:source_B                 1

IDs retain tskit’s zero-based indexing. `tskit_metadata()` and
`tskit_metadata_schema()` return exact raw bytes: decoding remains an
explicit caller decision because the C API does not itself interpret
metadata codecs. Source-population ancestry is valid only when the
simulation provenance makes its retained node populations authoritative.

## Native ownership

- `TreeSequence` owns one initialized `tsk_treeseq_t`.
- The external-pointer finalizer calls `tsk_treeseq_free()` exactly
  once.
- Native table memory is borrowed only for the duration of a `.Call()`.
- R data frames are copies requested explicitly by the user.
- Invalid or finalized pointers fail before native access.

## Vendored authority

The source package statically compiles tskit C API 1.3.1 and kastore
2.1.2 from tskit tag `1.0.3`, commit
`b62f75f04af81fbf5f0d6fb38953cb97b587737f`. One audited host-integration
patch routes impossible native invariant failures through an R error
instead of terminating the process. `tools/upstream.json`, the complete
manifests, and `patches/check.sh` prove the downstream source
reconstructs the pin exactly.

Rtskit is GPL-2 or later. The bundled tskit and kastore sources retain
their MIT licences and copyright notices.
