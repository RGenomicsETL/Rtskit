# Rtskit architecture

## Semantic authority

The documented tskit 1.x C API and `.trees` format are the native semantic
authority. Rtskit does not reproduce tskit algorithms in R and does not route
operations through another language runtime.

## Boundary

The native `rtskit_treeseq_handle` owns one initialized `tsk_treeseq_t`.
`TreeSequence` is the R-facing S7 value that contains the tagged external
pointer. Native functions borrow table columns only during `.Call()` and either
return compact scalar metadata or deliberately materialized R vectors.

Zero-based tskit IDs remain zero-based. Sizes are returned as R doubles where a
tskit size may exceed `INT_MAX`; materializers currently reject tables too
large for their data-frame row-name representation instead of narrowing IDs.

Metadata bytes and metadata schemas remain distinct. The initial interface does
not silently parse JSON or reinterpret arbitrary metadata.

## Native source policy

The package compiles pinned tskit and kastore C sources statically into the R
shared library. It does not discover a system tskit because the C project does
not promise shared-library ABI stability. Vendored sources are unmodified and
covered by a complete checksum inventory and upstream receipt.

The package interface is GPL-2 or later. Vendored MIT notices remain attached
to the upstream source.

## Scope discipline

Current operations load, dump, summarize, list samples, materialize node and
edge tables, and traverse tree intervals. New tables or algorithms are added
only with an R consumer, ownership contract, native test, and `.trees`
round-trip evidence. Simulation belongs in a separate layer built on table
construction and tree-sequence recording; it is not implied by tree-sequence
I/O alone.
