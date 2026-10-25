#!/usr/bin/env bash
# Compile the selected native Limbo tool using CC/ar and our freshly built iyacc.
# This compiler emits Dis/.sbl; it never executes Dis and needs no emu or CMake.
set -euo pipefail
root=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
output=${1:-"$root/../../build/limbo-bootstrap"}
mkdir -p "$output/generated" "$output/objects" "$output/tmp"
output=$(CDPATH= cd -- "$output" && pwd)
iyacc=${IYACC_EXECUTABLE:-"$output/../iyacc/iyacc"}
iyacc_root=${IYACC_ROOT:-"$output/../iyacc"}
[[ -x "$iyacc" && -f "$iyacc_root/utils/lib/yaccpar" ]] || {
    printf 'Build tools/iyacc first and supply IYACC_EXECUTABLE/IYACC_ROOT.\n' >&2
    exit 2
}
# Resolve relative caller overrides before changing to the generation directory.
iyacc=$(CDPATH= cd -- "$(dirname -- "$iyacc")" && pwd)/$(basename -- "$iyacc")
iyacc_root=$(CDPATH= cd -- "$iyacc_root" && pwd)
cc=${CC:-cc}
ar=${AR:-ar}
read -r -a cppflags <<< "${CPPFLAGS:-}"
read -r -a cflags <<< "${CFLAGS:-}"
read -r -a ldflags <<< "${LDFLAGS:-}"
includes=("-I$root/host/include" "-I$root/upstream/Linux/arm/include"
          "-I$root/upstream/include" "-I$root/upstream/libinterp" "-I$root/upstream/limbo")
export TMPDIR="$output/tmp" TEMP="$output/tmp" TMP="$output/tmp"
cp "$root/upstream/limbo/limbo.y" "$output/generated/limbo.y"
cp "$root/upstream/limbo/lex.c" "$output/generated/lex.c"
rm -f "$output/generated/y.tab.c" "$output/generated/y.tab.h"
(cd "$output/generated" && ROOT="$iyacc_root" "$iyacc" -d limbo.y)
[[ -s "$output/generated/y.tab.c" && -s "$output/generated/y.tab.h" ]]
"${PYTHON:-python3}" "$root/prepare_parser.py" --source "$output/generated/y.tab.c" \
    --output "$output/generated/safe-parser.c"

# Compile one source without flattening directory names or exposing shell evaluation.
compile_source() {
    local source=$1 object=$2
    "$cc" -std=gnu99 -O2 -fno-strict-aliasing "${cppflags[@]}" "${cflags[@]}" \
        "${includes[@]}" "-DINCPATH=\"$root/upstream/module\"" \
        -c "$source" -o "$object"
}

# Each archive has exactly the current explicit source list, without stale members.
compile_archive() {
    local group=$1 source object
    local objects=()
    while IFS= read -r source; do
        case "$source" in ''|'#'*) continue ;; esac
        object="$output/objects/${source//\//_}.o"
        if [[ "$source" == upstream/libmath/dtoa.c ]]; then
            compile_source "$output/generated/dtoa.c" "$object"
        else
            compile_source "$root/$source" "$object"
        fi
        objects+=("$object")
    done < "$root/$group.sources"
    rm -f "$output/$group.a"
    "$ar" rcs "$output/$group.a" "${objects[@]}"
}

"${PYTHON:-python3}" "$root/prepare_dtoa.py" --source "$root/upstream/libmath/dtoa.c" \
    --output "$output/generated/dtoa.c"
for group in lib9 bio support; do compile_archive "$group"; done
# Use the same checked constant-pool adaptation as the CMake build.
"${PYTHON:-python3}" "$root/prepare_ecom.py" --source "$root/upstream/limbo/ecom.c" \
    --output "$output/generated/ecom.c" --cells 4
objects=()
while IFS= read -r source; do
    case "$source" in ''|'#'*) continue ;; esac
    object="$output/objects/${source//\//_}.o"
    if [[ "$source" == upstream/limbo/ecom.c ]]; then
        compile_source "$output/generated/ecom.c" "$object"
    else
        compile_source "$root/$source" "$object"
    fi
    objects+=("$object")
done < "$root/compiler.sources"
for source in lex.c safe-parser.c; do
    object="$output/objects/generated_$source.o"
    compile_source "$output/generated/$source" "$object"
    objects+=("$object")
done
# lib9 and the floating conversion helpers have historical mutual references.
"$cc" "${ldflags[@]}" -o "$output/limbo-bootstrap" "${objects[@]}" \
    "$output/bio.a" "$output/support.a" "$output/lib9.a" "$output/support.a" -lm
printf 'Built %s\n' "$output/limbo-bootstrap"
