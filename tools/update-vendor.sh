#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
commit=b62f75f04af81fbf5f0d6fb38953cb97b587737f
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT INT HUP TERM

git clone --quiet --filter=blob:none https://github.com/tskit-dev/tskit.git "$tmp/tskit"
git -C "$tmp/tskit" checkout --quiet "$commit"

dst="$repo_root/src/vendor"
rm -rf "$dst/tskit" "$dst/kastore"
mkdir -p "$dst/tskit/tskit" "$dst/kastore"
cp "$tmp/tskit"/c/tskit/*.c "$tmp/tskit"/c/tskit/*.h "$dst/tskit/tskit/"
cp "$tmp/tskit/c/tskit.h" "$tmp/tskit/c/VERSION.txt" "$tmp/tskit/LICENSE" "$dst/tskit/"
cp "$tmp/tskit/c/subprojects/kastore/kastore.c" \
   "$tmp/tskit/c/subprojects/kastore/kastore.h" \
   "$tmp/tskit/c/subprojects/kastore/VERSION.txt" "$dst/kastore/"
# The tskit release carries the same MIT copyright for bundled kastore.
cp "$tmp/tskit/LICENSE" "$dst/kastore/LICENSE"

patch -d "$repo_root" -p1 < \
  "$repo_root/patches/0001-route-native-invariants-through-r.patch"
Rscript "$repo_root/tools/write-vendor-manifest.R"
printf 'Vendored tskit commit %s\n' "$commit"
