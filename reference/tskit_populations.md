# Materialize the population table

Population metadata remains opaque. Use
[`tskit_metadata()`](https://rgenomicsetl.github.io/Rtskit/reference/tskit_metadata.md)
to request its bytes explicitly.

## Usage

``` r
tskit_populations(x)
```

## Arguments

- x:

  A `TreeSequence`.

## Value

A data frame with zero-based population IDs and metadata lengths.
