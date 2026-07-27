# A native succinct tree sequence

`TreeSequence` owns one `tsk_treeseq_t` value through a native external
pointer. IDs returned by table accessors retain tskit's zero-based
indexing.

## Usage

``` r
TreeSequence(ptr = NULL, source = character(0))
```

## Arguments

- ptr:

  Internal live native pointer.

- source:

  Source path or `"<memory>"`.
