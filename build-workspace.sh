#!/usr/bin/env bash
# Optional workspace orchestration. Builds use selected, repository-owned SDK sources.
set -euo pipefail
ROOT=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
projects=vm,compilers,fir,linker,symbols,disvm,ide
build_root="$ROOT/build/workspace"
jobs=2
config=Release
tests=OFF
demo=0
demo_example=navigation
cmake=${CMAKE:-cmake}
ctest=${CTEST:-ctest}

# Display the interface without downloading, checking out or writing anything.
usage() {
    cat <<'HELP'
Usage: ./build-workspace.sh [options]
  --projects LIST    vm,compilers,fir,linker,symbols,disvm,ide (default all seven)
                    Individual frontends: as1,as2,as3,limbo
                    Explicit tools: cocor,cocor-limbo,limbo-bootstrap
  --build-root DIR   Default build/workspace; repeated builds are incremental
  --jobs N           Parallel jobs (default 2)
  --config NAME      CMake configuration (default Release)
  --tests            Build/run selected component tests; stop on failure
  --demo [NAME]      Run navigation (default) or history in a matching reference emu
  --help             Show help
Default generator: tools/coco-r-C, compiled locally when needed.
Default IDE bootstrap: tools/limbo-bootstrap, from bhgv/Inferno-OS-bhgv experiment.
DisVM builds the interpreter/JIT core archive; its full host runner is still pending.
Optional explicit SDK overrides:
  COCOR_ROOT + COCOR_EXECUTABLE     Supply BOTH or neither
  LIMBO_EXECUTABLE                 Native compiler override (bootstrap, not our FIR compiler)
  LIMBO_MODULE_DIR                 Default: bundled bootstrap module headers
  EMU_EXECUTABLE                   Required for IDE tests/demo, with matching module ABI
  CMAKE, CTEST, PYTHON              Optional host executable paths
No downloads, upstream changes, installation, cleanup or Git operations.
HELP
}

# Reject invalid requests/dependencies before touching build directories.
fail() {
    printf 'build-workspace: %s\n' "$*" >&2
    exit 2
}
while (($#)); do
    case "$1" in
        --help|-h) usage; exit 0 ;;
        --tests) tests=ON; shift ;;
        --demo)
            demo=1
            shift
            if (($#)) && [[ "$1" == navigation || "$1" == history ]]; then
                demo_example=$1
                shift
            fi ;;
        --projects|--build-root|--jobs|--config)
            (($# >= 2)) || fail "Missing value for $1"
            case "$1" in
                --projects) projects=$2 ;;
                --build-root) build_root=$2 ;;
                --jobs) jobs=$2 ;;
                --config) config=$2 ;;
            esac
            shift 2 ;;
        *) fail "Unknown argument: $1" ;;
    esac
done
[[ "$jobs" =~ ^[1-9][0-9]*$ ]] || fail 'jobs must be a positive integer'
[[ -n "$config" && -n "$build_root" ]] || fail 'Empty configuration or build root'
[[ -n "$projects" && "$projects" != ,* && "$projects" != *, && "$projects" != *,,* ]] || fail 'Invalid project list'
IFS=, read -r -a selected <<< "$projects"
for project in "${selected[@]}"; do
    case "$project" in
        vm|compilers|fir|linker|symbols|disvm|ide|as1|as2|as3|limbo|cocor|cocor-limbo|limbo-bootstrap) ;;
        *) fail "Unknown project: $project" ;;
    esac
done

# Exact membership only: list contents are never evaluated as shell code.
has() {
    local item
    for item in "${selected[@]}"; do
        [[ "$item" != "$1" ]] || return 0
    done
    return 1
}

# SDK paths are resolved relative to the invoking shell, not an implicit sibling checkout.
absolute() {
    case "$1" in
        /*) printf '%s\n' "$1" ;;
        *) printf '%s/%s\n' "$PWD" "$1" ;;
    esac
}
command -v "$cmake" >/dev/null || fail 'CMake not found'
if [[ "$tests" == ON || "$demo" == 1 ]]; then
    command -v "$ctest" >/dev/null || fail 'CTest not found'
fi
build_root=$(absolute "$build_root")
need_cocor=0
need_limbo=0
if has compilers || has fir || has symbols || has as1 || has as2 || has as3 || has limbo; then
    if [[ -n "${COCOR_ROOT:-}" || -n "${COCOR_EXECUTABLE:-}" ]]; then
        [[ -f "${COCOR_ROOT:-}/frames/scan_c.frm" ]] || fail 'COCOR_ROOT must contain frames/scan_c.frm'
        [[ -f "${COCOR_EXECUTABLE:-}" && -x "$COCOR_EXECUTABLE" ]] || fail 'Provide COCOR_ROOT and native COCOR_EXECUTABLE together'
        COCOR_ROOT=$(absolute "$COCOR_ROOT")
        COCOR_EXECUTABLE=$(absolute "$COCOR_EXECUTABLE")
    else
        need_cocor=1
        COCOR_ROOT="$ROOT/tools/coco-r-C/upstream"
        COCOR_EXECUTABLE="$build_root/cocor/cocor"
    fi
fi
if has ide || has cocor-limbo; then
    if [[ -n "${LIMBO_EXECUTABLE:-}" ]]; then
        [[ -f "$LIMBO_EXECUTABLE" && -x "$LIMBO_EXECUTABLE" ]] || fail 'LIMBO_EXECUTABLE must be a native executable'
        LIMBO_EXECUTABLE=$(absolute "$LIMBO_EXECUTABLE")
    else
        need_limbo=1
        LIMBO_EXECUTABLE="$build_root/limbo-bootstrap/limbo-bootstrap"
    fi
    LIMBO_MODULE_DIR=$(absolute "${LIMBO_MODULE_DIR:-$ROOT/tools/limbo-bootstrap/upstream/module}")
    [[ -f "$LIMBO_MODULE_DIR/sys.m" ]] || fail 'LIMBO_MODULE_DIR must contain sys.m'
fi
if has ide && [[ "$tests" == ON || "$demo" == 1 ]]; then
    [[ -f "${EMU_EXECUTABLE:-}" && -x "$EMU_EXECUTABLE" ]] || fail 'IDE execution requires explicit matching EMU_EXECUTABLE; disVM host runner is not yet complete'
    EMU_EXECUTABLE=$(absolute "$EMU_EXECUTABLE")
    command -v "${PYTHON:-python3}" >/dev/null || fail 'Python 3 not found for IDE host tests'
fi
[[ "$demo" != 1 ]] || has ide || fail '--demo requires ide in --projects'
mkdir -p "$build_root/tmp"
export TMPDIR="$build_root/tmp" TEMP="$build_root/tmp" TMP="$build_root/tmp"

# One configure/build/test path for tools and products. Selected projects keep
# standalone entry points; dependency tools are built first and never downloaded.
build_project() {
    local project=$1 source=$2
    shift 2
    "$cmake" -S "$ROOT/$source" -B "$build_root/$project" \
        "-DCMAKE_BUILD_TYPE=$config" "-DBUILD_TESTING=$tests" "$@"
    "$cmake" --build "$build_root/$project" --config "$config" --parallel "$jobs"
    if [[ "$tests" == ON ]]; then
        "$ctest" --test-dir "$build_root/$project" -C "$config" --output-on-failure --no-tests=error
    fi
}
if [[ "$need_cocor" == 1 ]] || has cocor; then
    build_project cocor tools/coco-r-C
fi
if [[ "$need_limbo" == 1 ]] || has limbo-bootstrap; then
    build_project limbo-bootstrap tools/limbo-bootstrap
fi
if has cocor-limbo; then
    build_project cocor-limbo tools/coco-r-limbo \
        "-DLIMBO_EXECUTABLE=$LIMBO_EXECUTABLE" "-DLIMBO_MODULE_DIR=$LIMBO_MODULE_DIR"
fi
for project in vm compilers as1 as2 as3 limbo fir linker symbols disvm ide; do
    has "$project" || continue
    args=()
    case "$project" in
        vm)
            source=flashVM
            args+=("-DFVM_BUILD_TESTS=$tests" -DFVM_BUILD_EXAMPLES=OFF -DFVM_BUILD_REFERENCE_RUNNER=ON) ;;
        compilers|fir|symbols|as1|as2|as3|limbo)
            source=compiler
            case "$project" in as1|as2|as3|limbo) source="compiler/$project" ;; esac
            [[ "$project" != fir ]] || source=fir/v1
            [[ "$project" != symbols ]] || source=compiler/limbo/symbols
            args+=("-DCOCOR_ROOT=$COCOR_ROOT" "-DCOCOR_EXECUTABLE=$COCOR_EXECUTABLE") ;;
        linker) source=linker ;;
        disvm) source=disVM ;;
        ide)
            source=ide/dis
            args+=("-DLIMBO_EXECUTABLE=$LIMBO_EXECUTABLE")
            if has symbols && [[ "$tests" == ON ]]; then
                args+=("-DSBLMAP_EXECUTABLE=$build_root/symbols/sblmap")
            fi
            if [[ "$tests" == ON || "$demo" == 1 ]]; then
                args+=(-DBUILD_TESTING=ON "-DEMU_EXECUTABLE=$EMU_EXECUTABLE" "-DLIMBO_MODULE_DIR=$LIMBO_MODULE_DIR")
            fi ;;
    esac
    build_project "$project" "$source" "${args[@]}"
done
if has ide && [[ "$tests" == ON ]]; then
    "${PYTHON:-python3}" -m unittest discover -s "$ROOT/ide/tests"
fi
if [[ "$demo" == 1 ]]; then
    demo_test=ide_dis_navigation_mode_
    [[ "$demo_example" != history ]] || demo_test=ide_dis_history_example_mode_
    "$ctest" --test-dir "$build_root/ide" -C "$config" -R "^${demo_test}[01]$" -V --no-tests=error
fi
printf 'Build complete: %s\n' "$build_root"
