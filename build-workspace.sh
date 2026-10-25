#!/usr/bin/env bash
# Optional workspace orchestration. Builds use selected, repository-owned SDK sources.
set -euo pipefail
ROOT=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)       # Absolute workspace source root; never used as a global HOME override.
projects=mk,iyacc,vm,compilers,fir,linker,symbols,disvm,disvm-host,ide # Default component selection; command-line --projects replaces it.
build_root="$ROOT/build/workspace"                                     # Default output tree; command-line --build-root replaces it.
jobs=2                                                                 # Parallel build jobs, validated before invoking the selected engine.
config=Release                                                         # Build configuration passed to CMake and CTest.
backend=auto                                                           # Requested build engine; auto probes only supported selections.
tests=OFF                                                              # Whether to build and run component checks.
demo=0                                                                 # Whether a reference-runtime IDE demonstration was requested.
demo_example=navigation                                                # Selected IDE demonstration name.
cmake=${CMAKE:-cmake}                                                  # Explicit CMAKE override or the command found on PATH.
ctest=${CTEST:-ctest}                                                  # Explicit CTEST override or the command found on PATH.
out_root="$ROOT/out"                                                   # Local product installation root; never a system prefix.
install_outputs=1                                                      # Install successful build products by default.
player=OFF                                                             # Optional real SDL2 window/audio/input host; requires its SDK.
vm_profiles=""                                                         # Optional comma-separated VM build profiles.

# Display the interface without downloading, checking out or writing anything.
# Parameters: none. Writes help to stdout; returns the cat command status.
usage() {
    cat <<'HELP'
Usage: ./build-workspace.sh [options]
  --projects LIST    vm,compilers,fir,linker,symbols,disvm,disvm-host,ide (default all eight)
                    Individual frontends: as1,as2,as3,limbo
                    Explicit tools: cocor,cocor-limbo,limbo-bootstrap,mk,iyacc
  --backend NAME    auto, cmake (3.10+), meson, mk or make; native subset documented
  --build-root DIR   Default build/workspace; repeated builds are incremental
  --jobs N           Parallel jobs (default 2)
  --config NAME      CMake configuration (default Release)
  --out DIR          Install local packages here (default out/)
  --no-install       Explicitly skip local product installation
  --vm-profiles LIST Build selected VM variants: default,minimal,agg,cairo
  --player           Build/install the native SDL2 player (requires an SDL2 SDK)
  --tests            Build/run selected component tests; stop on failure
  --demo [NAME]      Run navigation (default), history or replaceall in a matching reference emu
  --help             Show help
Default generator: tools/coco-r-C, compiled locally when needed.
Default IDE bootstrap: tools/limbo-bootstrap, from bhgv/Inferno-OS-bhgv experiment.
DisVM builds the interpreter/JIT core archive; its full host runner is still pending.
Optional explicit SDK overrides:
  COCOR_ROOT + COCOR_EXECUTABLE     Optional complete external Coco/R SDK pair
  COCOR_SOURCE                    auto (default), bundled, or strict external
  LIMBO_EXECUTABLE                 Native compiler override (bootstrap, not our FIR compiler)
  LIMBO_MODULE_DIR                 Default: bundled bootstrap module headers
  EMU_EXECUTABLE                   Optional reference runtime; no emu needed for building or compiler tests
  CMAKE, CTEST, PYTHON              Optional host executable paths
No downloads, upstream changes, system installation, cleanup or Git operations.
Successful products are installed locally under out/ unless --no-install is used.
HELP
}

# Reject invalid requests/dependencies before touching build directories.
# Parameters: all words form the diagnostic. Writes stderr and exits with status 2.
fail() {
    printf 'build-workspace: %s\n' "$*" >&2
    exit 2
}
while (($#)); do
    case "$1" in
        --help | -h)
            usage
            exit 0
            ;;
        --player)
            player=ON
            shift
            ;;
        --no-install)
            install_outputs=0
            shift
            ;;
        --tests)
            tests=ON
            shift
            ;;
        --demo)
            demo=1
            shift
            if (($#)) && [[ "$1" == navigation || "$1" == history || "$1" == replaceall ]]; then
                demo_example=$1
                shift
            fi
            ;;
        --projects | --build-root | --jobs | --config | --backend | --out | --vm-profiles)
            (($# >= 2)) || fail "Missing value for $1"
            case "$1" in
                --projects)
                    projects=$2
                    ;;
                --build-root)
                    build_root=$2
                    ;;
                --jobs)
                    jobs=$2
                    ;;
                --config)
                    config=$2
                    ;;
                --backend)
                    backend=$2
                    ;;
                --out)
                    out_root=$2
                    ;;
                --vm-profiles)
                    vm_profiles=$2
                    ;;
            esac
            shift 2
            ;;
        *)
            fail "Unknown argument: $1"
            ;;
    esac
done
[[ "$jobs" =~ ^[1-9][0-9]*$ ]] || fail 'jobs must be a positive integer'
[[ -n "$config" && -n "$build_root" ]] || fail 'Empty configuration or build root'
[[ -n "$projects" && "$projects" != ,* && "$projects" != *, && "$projects" != *,,* ]] || fail 'Invalid project list'
IFS=, read -r -a selected <<<"$projects"
for project in "${selected[@]}"; do
    case "$project" in
        vm | compilers | fir | linker | symbols | disvm | disvm-host | ide | as1 | as2 | as3 | limbo | cocor | cocor-limbo | limbo-bootstrap | mk | iyacc) ;;
        *)
            fail "Unknown project: $project"
            ;;
    esac
done

# Exact membership only: list contents are never evaluated as shell code.
# Parameters: $1 is a component name. Returns 0 when selected, otherwise 1.
has() {
    local item
    for item in "${selected[@]}"; do
        [[ "$item" != "$1" ]] || return 0
    done
    return 1
}

# SDK paths are resolved relative to the invoking shell, not an implicit sibling checkout.
# Parameters: $1 is a path. Prints its absolute spelling; does not test existence.
absolute() {
    case "$1" in
        /*)
            printf '%s\n' "$1"
            ;;
        *)
            printf '%s/%s\n' "$PWD" "$1"
            ;;
    esac
}
build_root=$(absolute "$build_root")
[[ -n "$out_root" ]] || fail 'Output root must not be empty'
out_root=$(absolute "$out_root")
if [[ -n "$vm_profiles" ]]; then
    has vm || fail '--vm-profiles requires vm in --projects'
    IFS=, read -r -a requested_profiles <<<"$vm_profiles"
    for profile in "${requested_profiles[@]}"; do
        case "$profile" in
            default | minimal | agg | cairo) ;;
            *)
                fail "Unknown VM profile: $profile"
                ;;
        esac
    done
fi
case "$backend" in auto | cmake | meson | mk | make) ;; *) fail "Unknown backend: $backend" ;; esac

# Probe versions without installing anything. Auto only selects an engine that
# supports the complete requested component set; explicit choices never fall back.
# Parameters: executable, banner kind, minimum major and minor. Returns 0 only
# for an available executable with a parsed version meeting the minimum.
version_at_least() {
    local executable=$1 expected=$2 minimum_major=$3 minimum_minor=$4 banner major minor
    command -v "$executable" >/dev/null 2>&1 || return 1
    banner=$("$executable" --version 2>/dev/null) || return 1
    if [[ "$expected" == cmake ]]; then
        banner=${banner#cmake version }
    fi
    [[ "$banner" =~ ^([0-9]{1,4})\.([0-9]{1,4})(\.|$) ]] || return 1
    major=$((10#${BASH_REMATCH[1]}))
    minor=$((10#${BASH_REMATCH[2]}))
    ((major > minimum_major || (major == minimum_major && minor >= minimum_minor)))
}
native_selection=1
for project in "${selected[@]}"; do
    case "$project" in mk | iyacc | limbo-bootstrap | cocor | compilers | as1 | as2 | as3 | limbo | linker) ;; *) native_selection=0 ;; esac
done
native_tests=1
for project in "${selected[@]}"; do
    case "$project" in mk | iyacc | limbo-bootstrap) ;; *) native_tests=0 ;; esac
done
if [[ "$demo" != 0 || ("$tests" != OFF && "$native_tests" == 0) ]]; then
    native_selection=0
fi
if [[ "$backend" == auto ]]; then
    if version_at_least "$cmake" cmake 3 10; then
        backend=cmake
    elif [[ "$native_selection" == 1 ]]; then
        if version_at_least "${MESON:-meson}" meson 0 54 && command -v "${NINJA:-ninja}" >/dev/null 2>&1; then
            backend=meson
        elif command -v "${MAKE:-make}" >/dev/null 2>&1; then
            backend=make
        elif command -v "${CC:-cc}" >/dev/null 2>&1 && command -v "${AR:-ar}" >/dev/null 2>&1; then
            backend=mk
        else
            fail 'No available native build engine/compiler toolchain'
        fi
    else
        fail 'This selection needs CMake 3.10+; native engines support compilers/linker/tools without --tests/--demo'
    fi
    printf 'Build backend: %s (auto)\n' "$backend"
fi
if [[ "$backend" != cmake ]]; then
    [[ "$demo" == 0 && ("$tests" == OFF || "$native_tests" == 1) ]] || fail 'Native --tests supports mk,iyacc,limbo-bootstrap only; --demo requires CMake and an executable Dis host'
    for project in "${selected[@]}"; do
        case "$project" in mk | iyacc | limbo-bootstrap | cocor | compilers | as1 | as2 | as3 | limbo | linker) ;;
        *)
            fail "Backend $backend does not yet build $project
            use CMake 3.10 for this component"
            ;;
        esac
    done
    export BUILD_ROOT="$build_root"
    cd "$ROOT"

    # Keep test execution outside the engine so Make, mk and Meson share the
    # same real-product checks and preserve build/test failures.
    # Parameters: none. Runs selected native checks when requested; failures propagate.
    finish_native() {
        if [[ "$install_outputs" == 1 ]]; then
            bash "$ROOT/tools/install-workspace.sh" --projects "$projects" --build-root "$build_root" --out "$out_root" --config "$config"
        fi
        if [[ "$tests" == ON ]]; then
            "$ROOT/tools/test-native-tools.sh" "${selected[@]}"
        fi
        printf 'Build complete: %s\n' "$build_root"
    }
    if [[ "$backend" == mk ]]; then
        "$ROOT/tools/mk/bootstrap.sh" "$build_root/mk"
        "$build_root/mk/mk" -f "$ROOT/mkfile" "${selected[@]}"
        finish_native
        exit 0
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
        finish_native
        exit 0
    fi
    command -v "${MAKE:-make}" >/dev/null || fail 'Make not found; explicit selection is not replaced automatically'
    "${MAKE:-make}" -f "$ROOT/Makefile" "${selected[@]}"
    finish_native
    exit 0
fi
need_cmake=0
for project in "${selected[@]}"; do
    case "$project" in mk | iyacc) ;; *) need_cmake=1 ;; esac
done
if [[ "$need_cmake" == 1 ]]; then
    command -v "$cmake" >/dev/null || fail 'CMake 3.10+ not found; --backend mk/make can build compilers, linker and native tools'
    if [[ "$tests" == ON || ("$demo" == 1 && -n "${EMU_EXECUTABLE:-}") ]]; then
        command -v "$ctest" >/dev/null || fail 'CTest not found'
    fi
fi
need_cocor=0
need_limbo=0
if has compilers || has fir || has symbols || has as1 || has as2 || has as3 || has limbo; then
    # The selector verifies the complete frame set before touching build products.
    source "$ROOT/tools/coco-r-C/select-sdk.sh"
    select_cocor_sdk "$ROOT" "$build_root" || exit 2
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
ide_runtime=0
if has ide && [[ "$tests" == ON || "$demo" == 1 ]]; then
    if [[ -n "${EMU_EXECUTABLE:-}" ]]; then
        [[ -f "$EMU_EXECUTABLE" && -x "$EMU_EXECUTABLE" ]] || fail 'EMU_EXECUTABLE must be executable when explicitly supplied'
        EMU_EXECUTABLE=$(absolute "$EMU_EXECUTABLE")
        ide_runtime=1
    else
        printf '%s\n' 'IDE: building Dis/debug products without emu; runtime tests are unavailable (embedded host pending).' >&2
    fi
    command -v "${PYTHON:-python3}" >/dev/null || fail 'Python 3 not found for IDE host tests'
fi
[[ "$demo" != 1 ]] || has ide || fail '--demo requires ide in --projects'
mkdir -p "$build_root/tmp"
export TMPDIR="$build_root/tmp" TEMP="$build_root/tmp" TMP="$build_root/tmp"

# Run from the build directory (CTest 3.10), and fail if no tests match.
# Parameters: build directory followed by optional CTest arguments. Returns the
# CTest status; empty selection is a fatal error instead of a false success.
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
# Parameters: output project name, repository-relative source and CMake options.
# Builds in its own directory and runs tests when requested; failures propagate.
build_project() {
    local project=$1 source=$2
    shift 2
    mkdir -p "$build_root/$project"
    # CMake 3.10 uses a positional source path from the build directory.
    (cd "$build_root/$project" && "$cmake" "$ROOT/$source" \
        "-DCMAKE_BUILD_TYPE=$config" "-DBUILD_TESTING=$tests" "$@")
    "$cmake" --build "$build_root/$project" --config "$config" -- "-j$jobs"
    if [[ "$install_outputs" == 1 ]]; then
        if [[ "$project" == vm-* ]]; then
            bash "$ROOT/tools/install-workspace.sh" --projects vm --build-root "$build_root" --out "$out_root" --config "$config" --vm-build "$build_root/$project"
        else
            bash "$ROOT/tools/install-workspace.sh" --projects "$project" --build-root "$build_root" --out "$out_root" --config "$config"
        fi
    fi
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
for project in vm compilers as1 as2 as3 limbo fir linker symbols disvm disvm-host ide; do
    has "$project" || continue
    args=()
    case "$project" in
        vm)
            source=flashVM
            args+=("-DFVM_BUILD_TESTS=$tests" -DFVM_BUILD_EXAMPLES=OFF -DFVM_BUILD_REFERENCE_RUNNER=ON "-DFVM_BUILD_DESKTOP_FRONTEND=$player")
            if [[ -n "$vm_profiles" ]]; then
                for profile in "${requested_profiles[@]}"; do
                    if [[ "$profile" == default ]]; then
                        definitions="$ROOT/flashVM/DEFS.txt"
                    else
                        definitions="$ROOT/flashVM/cmake/profiles/$profile.defs"
                    fi
                    if [[ "$profile" == minimal ]]; then
                        profile_player=OFF
                    else
                        profile_player=$player
                    fi
                    build_project "vm-$profile" "$source" "${args[@]}" "-DFVM_DEFS_FILE=$definitions" "-DFVM_BUILD_DESKTOP_FRONTEND=$profile_player"
                done
                continue
            fi
            ;;
        compilers | fir | symbols | as1 | as2 | as3 | limbo)
            source=compiler
            case "$project" in as1 | as2 | as3 | limbo) source="compiler/$project" ;; esac
            [[ "$project" != fir ]] || source=fir/v1
            [[ "$project" != symbols ]] || source=compiler/limbo/symbols
            args+=("-DCOCOR_ROOT=$COCOR_ROOT" "-DCOCOR_EXECUTABLE=$COCOR_EXECUTABLE")
            ;;
        linker)
            source=linker
            ;;
        disvm)
            source=disVM
            args+=("-DDVM_NATIVE_REGISTERS=${DVM_NATIVE_REGISTERS:-OFF}")
            args+=("-DDVM_FLASHVM_SOURCE_ROOT=$ROOT/flashVM")
            ;;
        disvm-host)
            source=disVM/host
            args+=("-DDVM_FLASHVM_SOURCE_ROOT=$ROOT/flashVM")
            args+=("-DDVM_FLASHVM_LIBRARIES=${DVM_FLASHVM_LIBRARIES:-}")
            ;;
        ide)
            source=ide/dis
            args+=("-DUIBRIDGE_SOURCE_ROOT=$ROOT/disVM" "-DIDE_AS_SOURCE_ROOT=$ROOT/ide/as")
            args+=("-DAS2_PARSE_EXECUTABLE=${AS2_PARSE_EXECUTABLE:-}" "-DNODE_EXECUTABLE=${NODE_EXECUTABLE:-}")
            args+=("-DLIMBO_EXECUTABLE=$LIMBO_EXECUTABLE")
            args+=("-DFIR_SOURCE_EXECUTABLE=")
            if has fir && [[ "$tests" == ON ]]; then
                args+=("-DFIR_SOURCE_EXECUTABLE=$build_root/fir/fir1_source")
            fi
            if has symbols && [[ "$tests" == ON ]]; then
                args+=("-DSBLMAP_EXECUTABLE=$build_root/symbols/sblmap")
            fi
            # Always reset the cached reference runner; a previous build must not
            # silently enable runtime execution after the override is removed.
            args+=("-DEMU_EXECUTABLE=" "-DLIMBO_MODULE_DIR=$LIMBO_MODULE_DIR")
            if [[ "$ide_runtime" == 1 ]]; then
                args+=(-DBUILD_TESTING=ON "-DEMU_EXECUTABLE=$EMU_EXECUTABLE")
            fi
            ;;
    esac
    build_project "$project" "$source" "${args[@]}"
done
if [[ "$install_outputs" == 1 ]]; then
    for project in mk iyacc; do
        if [[ -f "$build_root/$project/$project" || -f "$build_root/$project/$project.exe" ]]; then
            bash "$ROOT/tools/install-workspace.sh" --projects "$project" --build-root "$build_root" --out "$out_root" --config "$config"
        fi
    done
fi
if has ide && [[ "$tests" == ON ]]; then
    "${PYTHON:-python3}" -m unittest discover -s "$ROOT/ide/tests"
fi
if [[ "$demo" == 1 && "$ide_runtime" == 0 ]]; then
    printf 'Build complete: %s\n' "$build_root"
    printf '%s\n' 'NOT RUN: requested IDE demo needs an executable Dis host; compilation succeeded, runtime execution is not available.' >&2
    exit 3
fi
if [[ "$demo" == 1 ]]; then
    demo_test=ide_dis_navigation_mode_
    [[ "$demo_example" != history ]] || demo_test=ide_dis_history_example_mode_
    [[ "$demo_example" != replaceall ]] || demo_test=ide_dis_replaceall_example_mode_
    run_tests "$build_root/ide" -R "^${demo_test}[01]$" -V
fi
printf 'Build complete: %s\n' "$build_root"
