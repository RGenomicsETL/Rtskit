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
#>   trees: 1
#>   samples: 2
tskit_summary(trees)
#> $sequence_length
#> [1] 10
#> 
#> $trees
#> [1] 1
#> 
#> $samples
#> [1] 2
#> 
#> $individuals
#> [1] 0
#> 
#> $nodes
#> [1] 3
#> 
#> $edges
#> [1] 2
#> 
#> $sites
#> [1] 1
#> 
#> $mutations
#> [1] 1
#> 
#> $populations
#> [1] 0
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
#> 1  0     1    0         -1         -1
#> 2  1     1    0         -1         -1
#> 3  2     0    1         -1         -1
tskit_edges(trees)
#>   id left right parent child
#> 1  0    0    10      2     0
#> 2  1    0    10      2     1
tskit_trees(trees)
#>   index left right roots edges
#> 1     0    0    10     1     2
```

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
