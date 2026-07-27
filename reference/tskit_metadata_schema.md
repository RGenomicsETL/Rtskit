# Read an opaque metadata schema

The schema is returned exactly as stored. Convert or decode it
explicitly only when the declared codec is understood by the caller.

## Usage

``` r
tskit_metadata_schema(x, table)
```

## Arguments

- x:

  A `TreeSequence`.

- table:

  One of `"tree_sequence"`, `"reference_sequence"`, `"individuals"`,
  `"nodes"`, `"edges"`, `"migrations"`, `"sites"`, `"mutations"`, or
  `"populations"`.

## Value

A raw vector containing the table metadata schema.
