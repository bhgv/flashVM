#!/usr/bin/env bash
# Optional workspace orchestration. Builds use selected, repository-owned SDK sources.
set -euo pipefail
ROOT=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
projects=mk,iyacc,vm,compilers,fir,linker,symbols,disvm,ide
build_root="$ROOT/build/workspace"
jobs=2
config=Release
backend=auto
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
                    Explicit tools: cocor,cocor-limbo,limbo-bootstrap,mk,iyacc
  --backend NAME    auto, cmake (3.10+), meson, mk or make; native subset documented
  --build-root DIR   Default build/workspace; repeated builds are incremental
  --jobs N           Parallel jobs (default 2)
  --config NAME      CMake configuration (default Release)
  --tests            Build/run selected component tests; stop on failure
  --demo [NAME]      Run navigation (default), history or replaceall in a matching reference emu
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
            if (($#)) && [[ "$1" == navigation || "$1" == history || "$1" == replaceall ]]; then
                demo_example=$1
                shift
            fi ;;
        --projects|--build-root|--jobs|--config|--backend)
            (($# >= 2)) || fail "Missing value for $1"
            case "$1" in
                --projects) projects=$2 ;;
                --build-root) build_root=$2 ;;
                --jobs) jobs=$2 ;;
                --config) config=$2 ;;
                --backend) backend=$2 ;;
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
        vm|compilers|fir|linker|symbols|disvm|ide|as1|as2|as3|limbo|cocor|cocor-limbo|limbo-bootstrap|mk|iyacc) ;;
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
build_root=$(absolute "$build_root")
case "$backend" in auto|cmake|meson|mk|make) ;; *) fail "Unknown backend: $backend" ;; esac
# Probe versions without installing anything. Auto only selects an engine that
# supports the complete requested component set; explicit choices never fall back.
version_at_least() {
    local executable=$1 expected=$2 minimum_major=$3 minimum_minor=$4 banner major minor
    command -v "$executable" >/dev/null 2>&1 || return 1
    banner=$("$executable" --version 2>/dev/null) || return 1
    if [[ "$expected" == cmake ]]; then banner=${banner#cmake version }; fi
    [[ "$banner" =~ ^([0-9]{1,4})\.([0-9]{1,4})(\.|$) ]] || return 1
    major=$((10#${BASH_REMATCH[1]})); minor=$((10#${BASH_REMATCH[2]}))
    ((major > minimum_major || (major == minimum_major && minor >= minimum_minor)))
}
native_selection=1
for project in "${selected[@]}"; do
    case "$project" in mk|iyacc|cocor|compilers|as1|as2|as3|limbo|linker) ;; *) native_selection=0 ;; esac
done
if [[ "$tests" != OFF || "$demo" != 0 ]]; then native_selection=0; fi
if [[ "$backend" == auto ]]; then
    if version_at_least "$cmake" cmake 3 10; then backend=cmake
    elif [[ "$native_selection" == 1 ]]; then
        if version_at_least "${MESON:-meson}" meson 0 54 && command -v "${NINJA:-ninja}" >/dev/null 2>&1; then backend=meson
        elif command -v "${MAKE:-make}" >/dev/null 2>&1; then backend=make
        elif command -v "${CC:-cc}" >/dev/null 2>&1 && command -v "${AR:-ar}" >/dev/null 2>&1; then backend=mk
        else fail 'No available native build engine/compiler toolchain'; fi
    else fail 'This selection needs CMake 3.10+; native engines support compilers/linker/tools without --tests/--demo'; fi
    printf 'Build backend: %s (auto)\n' "$backend"
fi
if [[ "$backend" != cmake ]]; then
    [[ "$tests" == OFF && "$demo" == 0 ]] || fail 'Use the CMake 3.10 backend for --tests/--demo; native products can be tested directly'
    for project in "${selected[@]}"; do
        case "$project" in mk|iyacc|cocor|compilers|as1|as2|as3|limbo|linker) ;;
            *) fail "Backend $backend does not yet build $project; use CMake 3.10 for this component" ;;
        esac
    done
    export BUILD_ROOT="$build_root"
    cd "$ROOT"
    if [[ "$backend" == mk ]]; then
        "$ROOT/tools/mk/bootstrap.sh" "$build_root/mk"
        exec "$build_root/mk/mk" -f "$ROOT/mkfile" "${selected[@]}"
    fi
    if [[ "$backend" == meson ]]; then
        meson=${MESON:-meson}
        version_at_least "$meson" meson 0 54 || fail 'Meson 0.54+ not found; choose another available backend'
        command -v "${NINJA:-ninja}" >/dev/null || fail 'Meson backend requires Ninja (or NINJA=/path/ninja)'
        meson_dir="$build_root/meson"
        if [[ -f "$meson_dir/meson-private/coredata.dat" ]]; then
            "$meson" setup --reconfigure "$meson_dir" "$ROOT" "-Dnative_output_root=$build_root"
        else
            "$meson" setup "$meson_dir" "$ROOT" "-Dnative_output_root=$build_root"
        fi
        # Aggregate and individual selections can share outputs; serialize selected
        # run targets while allowing each native recipe to own its dependencies.
        for project in "${selected[@]}"; do
            "$meson" compile -C "$meson_dir" -j "$jobs" "$project"
        done
        exit 0
    fi
    command -v "${MAKE:-make}" >/dev/null || fail 'Make not found; explicit selection is not replaced automatically'
    exec "${MAKE:-make}" -f "$ROOT/Makefile" "${selected[@]}"
fi
need_cmake=0
for project in "${selected[@]}"; do
    case "$project" in mk|iyacc) ;; *) need_cmake=1 ;; esac
done
if [[ "$need_cmake" == 1 ]]; then
    command -v "$cmake" >/dev/null || fail 'CMake 3.10+ not found; --backend mk/make can build compilers, linker and native tools'
    if [[ "$tests" == ON || "$demo" == 1 ]]; then
        command -v "$ctest" >/dev/null || fail 'CTest not found'
    fi
fi
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
if has ide || has cocor-limbo || has limbo-bootstrap; then
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

# Run from the build directory (CTest 3.10), and fail if no tests match.
run_tests() {
    local directory=$1 listing
    shift
    listing=$(cd "$directory" && "$ctest" -N -C "$config" "$@")
    if [[ ! "$listing" =~ Total[[:space:]]Tests:[[:space:]]([0-9]+) ]] || [[ "${BASH_REMATCH[1]}" == 0 ]]; then
        fail "No tests found in $directory"
    fi
    (cd "$directory" && "$ctest" -C "$config" --output-on-failure "$@")
}

# One configure/build/test path for tools and products. Selected projects keep
# standalone entry points; dependency tools are built first and never downloaded.
build_project() {
    local project=$1 source=$2
    shift 2
    mkdir -p "$build_root/$project"
    # CMake 3.10 uses a positional source path from the build directory.
    (cd "$build_root/$project" && "$cmake" "$ROOT/$source" \
        "-DCMAKE_BUILD_TYPE=$config" "-DBUILD_TESTING=$tests" "$@")
    "$cmake" --build "$build_root/$project" --config "$config" -- "-j$jobs"
    if [[ "$tests" == ON ]]; then
        run_tests "$build_root/$project"
    fi
}
# Native host tools bootstrap without requiring CMake, Make or an installed mk.
if has iyacc || has limbo-bootstrap || [[ "$need_limbo" == 1 ]]; then
    BUILD_ROOT="$build_root" "$ROOT/tools/build-native.sh" iyacc
    IYACC_EXECUTABLE="$build_root/iyacc/iyacc"
    IYACC_ROOT="$build_root/iyacc"
elif has mk; then
    BUILD_ROOT="$build_root" "$ROOT/tools/build-native.sh" mk
fi
if [[ "$tests" == ON ]]; then
    if has mk || has iyacc || [[ "$need_limbo" == 1 ]]; then
        "${PYTHON:-python3}" "$ROOT/tools/mk/tests/smoke.py" "$build_root/mk/mk"
    fi
    if has iyacc || [[ "$need_limbo" == 1 ]]; then
        "${PYTHON:-python3}" "$ROOT/tools/iyacc/tests/smoke.py" "$build_root/iyacc/iyacc" \
            "$build_root/iyacc" "$ROOT/tools/limbo-bootstrap/upstream/limbo/limbo.y" \
            "$ROOT/tools/limbo-bootstrap/upstream/limbo/y.tab.h"
    fi
fi
if [[ "$need_cocor" == 1 ]] || has cocor; then
    build_project cocor tools/coco-r-C
fi
if [[ "$need_limbo" == 1 ]] || has limbo-bootstrap; then
    build_project limbo-bootstrap tools/limbo-bootstrap "-DIYACC_EXECUTABLE=${IYACC_EXECUTABLE:-}" "-DIYACC_ROOT=${IYACC_ROOT:-}"
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
            args+=("-DFIR_SOURCE_EXECUTABLE=")
            if has fir && [[ "$tests" == ON ]]; then
                args+=("-DFIR_SOURCE_EXECUTABLE=$build_root/fir/fir1_source")
            fi
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
    [[ "$demo_example" != replaceall ]] || demo_test=ide_dis_replaceall_example_mode_
    run_tests "$build_root/ide" -R "^${demo_test}[01]$" -V
fi
printf 'Build complete: %s\n' "$build_root"
