#!/usr/bin/env bash
# Optional native workspace orchestration. Components remain independent projects.
set -euo pipefail
ROOT=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
projects=vm,compilers,fir,linker,symbols,ide
build_root="$ROOT/build/workspace"
jobs=2
config=Release
tests=OFF
demo=0
cmake=${CMAKE:-cmake}
ctest=${CTEST:-ctest}

# Print the public interface without requiring any SDK or creating outputs.
usage() {
    cat <<'HELP'
Usage: ./build-workspace.sh [options]
  --projects LIST    Comma-separated vm,compilers,fir,linker,symbols,ide (default all)
                    Individual frontends: as1,as2,as3,limbo
  --build-root DIR   Build directory (default build/workspace)
  --jobs N           Parallel jobs (default 2)
  --config NAME      CMake configuration (default Release)
  --tests            Build and run selected component tests; stop on failure
  --demo             Build and run Limbo navigation example (requires ide)
  --help             Show this help
External SDK environment variables (native host executables):
  COCOR_ROOT, COCOR_EXECUTABLE       Required for compilers/fir/symbols
  LIMBO_EXECUTABLE                  Required for ide
  EMU_EXECUTABLE, LIMBO_MODULE_DIR  Required for --demo and ide --tests
  CMAKE, CTEST                      Optional executable paths
No SDK downloads, installation, cleanup, Git operations or scheduled jobs.
HELP
}

# Fail before invoking CMake when arguments or explicit dependencies are invalid.
fail() {
    printf 'build-workspace: %s\n' "$*" >&2
    exit 2
}
while (($#)); do
    case "$1" in
        --help|-h) usage; exit 0 ;;
        --tests) tests=ON; shift ;;
        --demo) demo=1; shift ;;
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
    case "$project" in vm|compilers|fir|linker|symbols|ide|as1|as2|as3|limbo) ;; *) fail "Unknown project: $project" ;; esac
done

# Membership uses exact names, never shell evaluation of the supplied list.
has() {
    local item
    for item in "${selected[@]}"; do
        [[ "$item" != "$1" ]] || return 0
    done
    return 1
}

# Normalize explicit SDK paths before changing working directory.
absolute() {
    case "$1" in /*) printf '%s\n' "$1" ;; *) printf '%s/%s\n' "$PWD" "$1" ;; esac
}
command -v "$cmake" >/dev/null || fail 'CMake not found'
if [[ "$tests" == ON || "$demo" == 1 ]]; then
    command -v "$ctest" >/dev/null || fail 'CTest not found'
fi
if has compilers || has fir || has symbols || has as1 || has as2 || has as3 || has limbo; then
    [[ -f "${COCOR_ROOT:-}/frames/scan_c.frm" ]] || fail 'Set COCOR_ROOT to bhgv/coco-r-C'
    [[ -f "${COCOR_EXECUTABLE:-}" && -x "$COCOR_EXECUTABLE" ]] || fail 'Set COCOR_EXECUTABLE to native cocor'
    COCOR_ROOT=$(absolute "$COCOR_ROOT")
    COCOR_EXECUTABLE=$(absolute "$COCOR_EXECUTABLE")
fi
if has ide; then
    [[ -f "${LIMBO_EXECUTABLE:-}" && -x "$LIMBO_EXECUTABLE" ]] || fail 'Set LIMBO_EXECUTABLE to native limbo'
    LIMBO_EXECUTABLE=$(absolute "$LIMBO_EXECUTABLE")
    if [[ "$tests" == ON || "$demo" == 1 ]]; then
        [[ -f "${EMU_EXECUTABLE:-}" && -x "$EMU_EXECUTABLE" ]] || fail 'Set EMU_EXECUTABLE to native reference emu'
        [[ -f "${LIMBO_MODULE_DIR:-}/sys.m" ]] || fail 'Set LIMBO_MODULE_DIR containing sys.m'
        EMU_EXECUTABLE=$(absolute "$EMU_EXECUTABLE")
        LIMBO_MODULE_DIR=$(absolute "$LIMBO_MODULE_DIR")
    fi
fi
[[ "$demo" != 1 ]] || has ide || fail '--demo requires ide in --projects'
build_root=$(absolute "$build_root")
mkdir -p "$build_root/tmp"
export TMPDIR="$build_root/tmp" TEMP="$build_root/tmp" TMP="$build_root/tmp"

# Configure/build in fixed dependency order. Repeated calls reuse CMake outputs.
# Failure preserves partial artifacts for inspection and stops subsequent work.
for project in vm compilers as1 as2 as3 limbo fir linker symbols ide; do
    has "$project" || continue
    args=("-DCMAKE_BUILD_TYPE=$config" "-DBUILD_TESTING=$tests")
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
        ide)
            source=ide/dis
            args+=("-DLIMBO_EXECUTABLE=$LIMBO_EXECUTABLE")
            if [[ "$tests" == ON || "$demo" == 1 ]]; then
                args+=(-DBUILD_TESTING=ON "-DEMU_EXECUTABLE=$EMU_EXECUTABLE" "-DLIMBO_MODULE_DIR=$LIMBO_MODULE_DIR")
            fi ;;
    esac
    "$cmake" -S "$ROOT/$source" -B "$build_root/$project" "${args[@]}"
    "$cmake" --build "$build_root/$project" --config "$config" --parallel "$jobs"
    if [[ "$tests" == ON ]]; then
        "$ctest" --test-dir "$build_root/$project" -C "$config" --output-on-failure --no-tests=error
    fi
done
if [[ "$demo" == 1 ]]; then
    "$ctest" --test-dir "$build_root/ide" -C "$config" -R '^ide_dis_navigation_mode_[01]$' -V --no-tests=error
fi
printf 'Build complete: %s\n' "$build_root"
