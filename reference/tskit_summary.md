# Summarize a succinct tree sequence

Summarize a succinct tree sequence

## Usage

``` r
tskit_summary(x)
```

## Arguments

- x:

  A `TreeSequence`.

## Value

A named list of sequence length and table/tree counts. Counts are
doubles because tskit sizes can exceed R's integer range.
