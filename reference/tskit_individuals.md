# Materialize the individual table

Ragged locations and parent IDs are returned as list columns. Individual
metadata remains opaque and is available through
[`tskit_metadata()`](https://rgenomicsetl.github.io/Rtskit/reference/tskit_metadata.md).

## Usage

``` r
tskit_individuals(x)
```

## Arguments

- x:

  A `TreeSequence`.

## Value

A data frame with zero-based individual IDs, flags, location and parent
list columns, and metadata lengths.
