# Read opaque metadata bytes

Rtskit deliberately returns raw bytes and does not infer a codec from a
schema. Row tables return a list of raw vectors in the requested ID
order. Top-level tree-sequence and reference-sequence metadata return
one raw vector and do not accept `id`.

## Usage

``` r
tskit_metadata(x, table, id = NULL)
```

## Arguments

- x:

  A `TreeSequence`.

- table:

  One of `"tree_sequence"`, `"reference_sequence"`, `"individuals"`,
  `"nodes"`, `"edges"`, `"migrations"`, `"sites"`, `"mutations"`, or
  `"populations"`.

- id:

  Optional zero-based row IDs. `NULL` requests every row of a row table.

## Value

A raw vector for top-level metadata or a list of raw vectors for a row
table.
