#!/bin/sh
set -eu
repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT INT HUP TERM
mkdir -p "$tmp/src"
cp -R "$repo_root/src/vendor" "$tmp/src/vendor"
rm -f "$tmp/src/vendor/manifest.tsv"
patch -s -R -d "$tmp" -p1 < \
  "$repo_root/patches/0001-route-native-invariants-through-r.patch"
Rscript "$repo_root/tools/check-upstream-vendor.R" "$tmp"
