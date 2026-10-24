#!/usr/bin/env bash
# Direct POSIX C builds used by Make/mk; no CMake, Meson or installed Coco/R SDK.
set -euo pipefail
ROOT=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
build_root=${BUILD_ROOT:-"$ROOT/build/native"}
[[ "$build_root" == /* ]] || build_root="$PWD/$build_root"
cc=${CC:-cc}
ar=${AR:-ar}
read -r -a user_cflags <<< "${CFLAGS:-}"
read -r -a user_cppflags <<< "${CPPFLAGS:-}"
read -r -a user_ldflags <<< "${LDFLAGS:-}"
declare -A built
mkdir -p "$build_root/tmp"
export TMPDIR="$build_root/tmp" TEMP="$build_root/tmp" TMP="$build_root/tmp"
includes=()
defines=()

# Fail on unsupported selections before touching products; never silently omit a project.
for component in "$@"; do
    case "$component" in
        mk|iyacc|cocor|as1|as2|as3|limbo|compilers|linker) ;;
        *) printf 'Native backend does not yet build %s; use the CMake 3.10 path for this component.\n' "$component" >&2; exit 2 ;;
    esac
done

# Compile an owned static library from explicit source paths and current private flags.
compile_library() {
    local output=$1 source object index=0
    shift
    local objects=()
    mkdir -p "$output.objects"
    for source in "$@"; do
        object="$output.objects/$index.o"
        "$cc" -std=c99 -O2 "${user_cppflags[@]}" "${user_cflags[@]}" \
            "${includes[@]}" "${defines[@]}" -c "$source" -o "$object"
        objects+=("$object")
        index=$((index + 1))
    done
    rm -f "$output"
    "$ar" rcs "$output" "${objects[@]}"
}

# Link a real CLI against its owned archive; a failed compiler/linker stops the script.
link_program() {
    local output=$1 source=$2 archive=$3
    "$cc" -std=c99 -O2 "${user_cppflags[@]}" "${user_cflags[@]}" \
        "${includes[@]}" "${defines[@]}" "$source" "$archive" \
        "${user_ldflags[@]}" -o "$output"
}

# Prefix every generated scanner/parser global, matching the component's CMake contract.
parser_defines() {
    local prefix=$1 symbol
    defines=(-DCUSTOM_BUFF)
    for symbol in Custom_Error GenError Parse SemError Successful SynError \
        S_Col S_CurrLine S_CurrentCh S_Get S_GetName S_GetString S_Len S_Line \
        S_LineStart S_NextCol S_NextLen S_NextLine S_NextPos S_Pos S_Reset S_src; do
        defines+=("-D$symbol=${prefix}_$symbol")
    done
}

# Generate fresh files with the same authoritative Coco/R grammar and failure rules.
generate_parser() {
    local name=$1 source=$2 output=$3 suffix
    mkdir -p "$output"
    for suffix in p.c s.c p.h s.h c.h e.h; do rm -f "$output/$name$suffix"; done
    cp "$source" "$output/$name.atg"
    if ! (cd "$output" && CRFRAMES="$ROOT/tools/coco-r-C/upstream/frames" \
        "$build_root/cocor/cocor" "$name.atg") > "$output/generation.log" 2>&1; then
        cat "$output/generation.log" >&2; return 1
    fi
    if LC_ALL=C grep -Eiq '\(e[0-9]+\)|ll\(1\) error|not ll\(1\)' "$output/generation.log"; then
        cat "$output/generation.log" >&2; return 1
    fi
    for suffix in p.c s.c p.h s.h c.h e.h; do
        [[ -f "$output/$name$suffix" ]] || { cat "$output/generation.log" >&2; return 1; }
    done
}

# Build dependencies once per invocation; all products remain under build_root.
build_component() {
    local component=$1 output="$build_root/$1" source name
    [[ "${built[$component]:-}" != yes ]] || return 0
    case "$component" in
        mk)
            "$ROOT/tools/mk/bootstrap.sh" "$output" ;;
        iyacc)
            build_component mk
            MK_SUPPORT_ROOT="$build_root/mk" "$ROOT/tools/iyacc/bootstrap.sh" "$output" ;;
        cocor)
            mkdir -p "$output"
            source="$ROOT/tools/coco-r-C/upstream/sources"
            includes=("-I$source"); defines=()
            compile_library "$output/libcocor.a" "$source"/{cr,cra,crp,crs,crt,crx,crf,collect,set}.c
            # Coco/R's main lives in the archive; the C runtime requests it normally.
            "$cc" "${user_ldflags[@]}" "$output/libcocor.a" -o "$output/cocor" ;;
        compilers)
            for name in as1 as2 as3 limbo; do build_component "$name"; done ;;
        as1|as2|as3|limbo)
            build_component cocor
            source="$ROOT/compiler/$component/frontend"
            generate_parser "$component" "$source/$component.atg" "$output/generated"
            includes=("-I$source" "-I$output/generated"); parser_defines "$component"
            local sources=("$source/api.c" "$source/parser.c" "$source/scanner.c")
            if [[ "$component" == limbo ]]; then
                sources+=("$source/tree.c" "$source/interface.c" "$source/signature.c")
            fi
            compile_library "$output/libfrontend.a" "${sources[@]}"
            link_program "$output/${component}_parse" "$source/main.c" "$output/libfrontend.a"
            if [[ "$component" == limbo ]]; then
                for name in tree interface bindings; do
                    local driver="$source/${name}_main.c"
                    [[ "$name" != bindings ]] || driver="$source/bindings.c"
                    link_program "$output/limbo_$name" "$driver" "$output/libfrontend.a"
                done
            fi ;;
        linker)
            source="$ROOT/linker"
            mkdir -p "$output"
            includes=("-I$source/include"); defines=()
            compile_library "$output/libswl_core.a" "$source/src"/{linker,archive,assets,inspect}.c
            link_program "$output/swl" "$source/src/main.c" "$output/libswl_core.a" ;;
    esac
    built[$component]=yes
    printf 'Native build complete: %s\n' "$component"
}

for component in "$@"; do build_component "$component"; done
