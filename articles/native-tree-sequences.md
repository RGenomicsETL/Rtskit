# Native tree sequences in R

`Rtskit` keeps a succinct tree sequence in an owned native
`tsk_treeseq_t`. R receives copied summaries or table columns only when
an accessor requests them. No Python runtime or system tskit shared
library is involved.

``` r

library(Rtskit)

trees <- tskit_example()
trees
#> <Rtskit::TreeSequence>
#>   source: <memory>
#>   sequence length: 10
#>   trees: 2
#>   samples: 2
tskit_summary(trees)
#> $sequence_length
#> [1] 10
#> 
#> $trees
#> [1] 2
#> 
#> $samples
#> [1] 2
#> 
#> $individuals
#> [1] 1
#> 
#> $nodes
#> [1] 4
#> 
#> $edges
#> [1] 4
#> 
#> $sites
#> [1] 1
#> 
#> $mutations
#> [1] 1
#> 
#> $populations
#> [1] 2
#> 
#> $migrations
#> [1] 0
#> 
#> $provenances
#> [1] 0
```

## Tables and coordinates

Node, edge, and tree IDs preserve tskit’s zero-based indexing. Genomic
intervals are half-open: an edge over `[left, right)` includes `left`
and excludes `right`.

``` r

tskit_nodes(trees)
#>   id flags time population individual
#> 1  0     1    0         -1          0
#> 2  1     1    0         -1          0
#> 3  2     0    1          0         -1
#> 4  3     0    1          1         -1
tskit_edges(trees)
#>   id left right parent child
#> 1  0    0     5      2     0
#> 2  1    0     5      2     1
#> 3  2    5    10      3     0
#> 4  3    5    10      3     1
tskit_populations(trees)
#>   id metadata_length
#> 1  0              19
#> 2  1              19
tskit_individuals(trees)
#>   id flags location parents metadata_length
#> 1  0     0 1.5, 2.5                      17
tskit_trees(trees)
#>   index left right roots edges
#> 1     0    0     5     1     2
#> 2     1    5    10     1     2
```

## Opaque metadata

Metadata and its schema remain separate raw byte sequences. Decode them
only when the stored schema declares a codec the calling analysis
understands.

``` r

rawToChar(tskit_metadata_schema(trees, "populations"))
#> [1] "{\"codec\":\"json\",\"type\":\"object\"}"
lapply(tskit_metadata(trees, "populations"), rawToChar)
#> [[1]]
#> [1] "{\"name\":\"source_A\"}"
#> 
#> [[2]]
#> [1] "{\"name\":\"source_B\"}"
```

## Source-population ancestry

For simulations that retain authoritative source populations,
[`tskit_ancestry_intervals()`](https://rgenomicsetl.github.io/Rtskit/reference/tskit_ancestry_intervals.md)
walks each focal sample lineage to the first node in a declared source
population. Adjacent marginal trees are merged while the source remains
unchanged.

``` r

tskit_ancestry_intervals(
  trees,
  c("panel:source_A" = 0L, "panel:source_B" = 1L)
)
#>   sample left right         source source_population
#> 1      0    0     5 panel:source_A                 0
#> 2      0    5    10 panel:source_B                 1
#> 3      1    0     5 panel:source_A                 0
#> 4      1    5    10 panel:source_B                 1
```

This operation does not infer ancestry labels. The source IDs and their
truth semantics must come from the simulation manifest.

## Round trips

[`tskit_dump()`](https://rgenomicsetl.github.io/Rtskit/reference/tskit_dump.md)
writes the native value directly through the tskit C API.
[`tskit_load()`](https://rgenomicsetl.github.io/Rtskit/reference/tskit_load.md)
creates a new, independently owned native value.

``` r

path <- tempfile(fileext = ".trees")
tskit_dump(trees, path)
copy <- tskit_load(path)
identical(tskit_summary(copy), tskit_summary(trees))
#> [1] TRUE
unlink(path)
```

The package deliberately does not reinterpret arbitrary table metadata.
Metadata schemas and bytes require explicit schema-aware handling rather
than implicit JSON conversion.
