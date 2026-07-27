# Extract source-population ancestry intervals

For every focal sample and marginal tree, this function walks toward the
root and records the first node whose population is one of
`source_populations`. Adjacent tree intervals with the same source are
merged. This is exact ancestry truth only when the simulation's
population and node-retention semantics make those source populations
authoritative.

## Usage

``` r
tskit_ancestry_intervals(
  x,
  source_populations,
  samples = NULL,
  require_resolved = TRUE
)
```

## Arguments

- x:

  A `TreeSequence`.

- source_populations:

  A non-empty vector of zero-based source-population IDs. Names, when
  supplied, become the returned source labels and should be
  dataset-qualified.

- samples:

  Zero-based focal sample-node IDs. `NULL` uses every sample.

- require_resolved:

  Whether to fail if a focal lineage reaches a root without encountering
  a declared source population.

## Value

A data frame of sample IDs, half-open intervals, source labels, and
source-population IDs, grouped by sample and ordered by coordinate.

## References

Haller et al. (2019), *Tree-sequence recording in SLiM opens new
horizons for forward-time simulation of whole genomes*,
[doi:10.1111/1755-0998.12968](https://doi.org/10.1111/1755-0998.12968) .
The published local-ancestry example is pinned at SLiMTreeSeqPub commit
`6715c28b02942bc4757c9f8bcab133ad4a0bfcfb`.
