#!/bin/sh
# Rewrites the co-op's layer native/coop/patches/<path>.patch from the edited native/build/patched/<path>:
# the difference from the port's own patched file (dw3_decomp/<path> + native/patches/<path>.patch), so the
# port's patches (frozen at its v0.18.9) stay as they were.
# The decomp's files are CRLF (Git's autocrlf) and the patched copies LF, hence --strip-trailing-cr.
# Usage (Git Bash): sh native/scripts/mkpatch.sh src/fieldstg/field.c [src/fieldstg/actor_control.c ...]
set -e
root=$(cd "$(dirname "$0")/../.." && pwd) # the project's folder: native/, dw3_decomp/
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT
for rel in "$@"; do
    up="$root/native/patches/$rel.patch"
    out="$root/native/coop/patches/$rel.patch"
    base="$tmpdir/base"
    if [ -f "$up" ]; then
        patch -s -o "$base" "$root/dw3_decomp/$rel" "$up"
    else
        cp "$root/dw3_decomp/$rel" "$base" # a file the port leaves as it is: the layer goes over the decomp's
    fi
    mkdir -p "$(dirname "$out")"
    # diff exits 1 when the files differ, which is the normal case
    diff -u --strip-trailing-cr --label "a/$rel" --label "b/$rel" "$base" "$root/native/build/patched/$rel" > "$out.tmp" || [ $? -eq 1 ]
    if [ ! -s "$out.tmp" ]; then
        rm -f "$out.tmp"
        echo "$rel: no co-op changes, layer left as it was" >&2
        exit 1
    fi
    mv "$out.tmp" "$out"
    echo "$rel: $(grep -c '^@@' "$out") hunks"
done
