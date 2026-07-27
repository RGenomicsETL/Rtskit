# Vendored native patches

Rtskit vendors the C sources from tskit tag `1.0.3`, commit
`b62f75f04af81fbf5f0d6fb38953cb97b587737f`.

`0001-route-native-invariants-through-r.patch` uses extension hooks explicitly
anticipated by the tskit headers to route impossible internal invariant failures
through an R error rather than `abort()`. It also removes the default `stdout`
debug stream in R-package builds; Rtskit does not expose tskit debug flags.
Normal malformed input remains represented by tskit error codes and converted
at the `.Call()` boundary.

Run `patches/check.sh` to reverse the ordered downstream patch and verify the
complete selected source inventory against `tools/upstream-manifest.tsv`.
