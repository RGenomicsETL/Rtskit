
# Rtskit

`Rtskit` is a native R interface to succinct tree sequences through the
stable [`tskit` C API](https://tskit.dev/tskit/docs/stable/c-api.html).
It does not use Python, `reticulate`, Rust, Java, or a system tskit
shared library.

The initial interface keeps each `tsk_treeseq_t` under one native
external pointer and exposes deliberate materialization operations for
summaries, sample nodes, node and edge tables, and tree intervals.

``` r
library(Rtskit)

trees <- tskit_example()
trees
```

    ## <Rtskit::TreeSequence>
    ##   source: <memory>
    ##   sequence length: 10
    ##   trees: 1
    ##   samples: 2

``` r
tskit_nodes(trees)
```

    ##   id flags time population individual
    ## 1  0     1    0         -1         -1
    ## 2  1     1    0         -1         -1
    ## 3  2     0    1         -1         -1

``` r
tskit_edges(trees)
```

    ##   id left right parent child
    ## 1  0    0    10      2     0
    ## 2  1    0    10      2     1

``` r
tskit_trees(trees)
```

    ##   index left right roots edges
    ## 1     0    0    10     1     2

IDs retain tskit’s zero-based indexing. Table metadata remains opaque
bytes until an explicit schema-aware interface is implemented; the C API
does not itself decode or validate metadata schemas.

## Native ownership

- `TreeSequence` owns one initialized `tsk_treeseq_t`.
- The external-pointer finalizer calls `tsk_treeseq_free()` exactly
  once.
- Native table memory is borrowed only for the duration of a `.Call()`.
- R data frames are copies requested explicitly by the user.
- Invalid or finalized pointers fail before native access.

## Vendored authority

The source package statically compiles unmodified tskit C API 1.3.1 and
kastore 2.1.2 from tskit tag `1.0.3`, commit
`b62f75f04af81fbf5f0d6fb38953cb97b587737f`. `tools/upstream.json`
records the pin and `src/vendor/manifest.tsv` audits every vendored
file.

Rtskit is GPL-2 or later. The bundled tskit and kastore sources retain
their MIT licences and copyright notices.
