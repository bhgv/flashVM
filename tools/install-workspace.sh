#!/usr/bin/env bash
# Copyright (c) 2026 bhgv
# EN: Install selected products; component-specific payloads live under tools/install/.
# RU: Установка выбранных продуктов; состав компонентов вынесен в tools/install/.
set -euo pipefail
ROOT=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd) # Workspace source root.
BUILD="$ROOT/build/workspace"                                       # Existing build tree, never cleaned here.
OUT="$ROOT/out"                                                     # User-facing local installation prefix.
PROJECTS=""                                                         # Explicit comma-separated package selection.
CONFIG=Release                                                      # Multi-configuration build directory spelling.
VM_BUILD=""                                                         # Optional separately built FlashVM variant.

# Reject an incomplete package instead of silently reporting a successful installation.
# Parameters: diagnostic words. Returns: never; writes stderr and exits with status 1.
# RU: Сообщить об ошибке из аргументов и завершить установку с кодом 1.
fail() {
    printf 'install-workspace: %s\n' "$*" >&2
    exit 1
}

# Copy one known product, retaining ordinary data permissions or setting executable mode.
# Parameters: source, destination, optional mode. Returns: cp/chmod failure propagates.
# RU: Копировать source в destination с mode; отсутствие источника — ошибка.
copy_file() {
    local source=$1 destination=$2 mode=${3:-644}
    [[ -f "$source" ]] || fail "Missing product: $source"
    mkdir -p "$(dirname -- "$destination")"
    cp -p -- "$source" "$destination"
    chmod "$mode" "$destination"
}

# Copy an explicitly selected SDK/example directory; no source checkout is removed.
# Parameters: source directory and destination directory. Returns: copy failure propagates.
# RU: Копировать дерево source в destination; чужие файлы не удалять.
copy_tree() {
    local source=$1 destination=$2
    [[ -d "$source" ]] || fail "Missing SDK directory: $source"
    mkdir -p "$destination"
    cp -R -- "$source/." "$destination/"
}

# Locate a named executable in native, CMake or multi-configuration output layouts.
# Parameters: build directory and target basename. Returns: prints a path, or fails visibly.
# RU: В base найти name/CONFIG и вернуть путь; отсутствие программы — ошибка.
program() {
    local base=$1 name=$2 candidate found scan_base
    for candidate in "$base/$CONFIG/$name.exe" "$base/$CONFIG/$name" "$base/$name.exe" "$base/$name"; do
        if [[ -f "$candidate" ]]; then
            printf '%s\n' "$candidate"
            return 0
        fi
    done
    scan_base=$(cd "$base" && pwd -P) || fail "Missing executable build directory: $base"
    found=$(find "$scan_base" -path "$OUT" -prune -o -type f \( -name "$name" -o -name "$name.exe" \) ! -path '*/CMakeFiles/*' -print -quit)
    [[ -n "$found" ]] || fail "Executable $name was not built under $base"
    printf '%s\n' "$found"
}

# Install the actual named program; PE outputs keep their .exe suffix.
# Parameters: build directory, target basename, package directory. Returns: copy failure propagates.
# RU: Установить программу name из base в destination с учётом .exe.
install_program() {
    local source name=$2 destination=$3
    source=$(program "$1" "$name")
    if [[ "$source" == *.exe ]]; then
        name="$name.exe"
    fi
    copy_file "$source" "$destination/$name" 755
}

# Load Dis64 packaging separately from the component dispatch.
source "$ROOT/tools/install/dis64-envelope.sh"
source "$ROOT/tools/install/runtime.sh"
source "$ROOT/tools/install/runtime-payload.sh"
source "$ROOT/tools/install/host.sh"
source "$ROOT/tools/install/fir.sh"

# Preserve first-party licensing plus selected upstream notices rather than relabeling code.
# Parameters: package directory and component source directory. Returns: copy failure propagates.
# RU: Скопировать лицензии source в destination без смены авторства.
notices() {
    local destination=$1 source=$2 file relative
    copy_file "$ROOT/LICENSE" "$destination/LICENSES/FlashVM-MIT.txt"
    find "$source" -path "$OUT" -prune -o -type f \( -iname 'license*' -o -iname 'copying*' -o -name NOTICE -o -name 'NOTICE.*' -o -path '*/lib/legal/*' \) ! -path '*/build/*' -print0 |
        while IFS= read -r -d '' file; do
            relative=${file#"$source/"}
            copy_file "$file" "$destination/LICENSES/$relative"
        done
    if [[ -f "$source/UPSTREAM.json" ]]; then
        copy_file "$source/UPSTREAM.json" "$destination/UPSTREAM.json"
    fi
}

# Install libraries without flattening their relative paths or mixing graphics variants.
# Parameters: build directory and package directory. Returns: copy failure propagates.
# RU: Скопировать библиотеки base в destination, сохранив структуру.
libraries() {
    local base=$1 destination=$2 file relative scan_base
    # A build-root entry may be a symlink to an already tested build. Traverse its
    # actual directory so SDK archives are included as well as directly located executables.
    scan_base=$(cd "$base" && pwd -P) || fail "Missing library build directory: $base"
    find "$scan_base" -path "$OUT" -prune -o \( -type f -o -type l \) \( -name '*.a' -o -name '*.lib' -o -name '*.dll' -o -name '*.so' -o -name '*.so.*' -o -name '*.dylib' \) ! -path '*/CMakeFiles/*' -print0 |
        while IFS= read -r -d '' file; do
            relative=${file#"$scan_base/"}
            copy_file "$file" "$destination/lib/$relative"
        done
}

# Package one standalone parser with two inputs and its own public API declaration.
# Parameters: language name and build directory. Returns: complete package or an error.
frontend() {
    local language=$1 base=$2 destination="$OUT/$1" sample
    mkdir -p "$destination/examples"
    install_program "$base" "${language}_parse" "$destination"
    libraries "$base" "$destination"
    copy_file "$ROOT/compiler/$language/frontend/frontend.h" "$destination/include/frontend.h"
    if [[ "$language" == limbo ]]; then
        copy_tree "$ROOT/compiler/limbo/examples/source-position" "$destination/examples/source-position"
        copy_file "$ROOT/compiler/limbo/frontend/SOURCE-POSITIONS.md" "$destination/SOURCE-POSITIONS.md"
        copy_file "$ROOT/compiler/limbo/frontend/SOURCE-POSITIONS.ru.md" "$destination/SOURCE-POSITIONS.ru.md"
        copy_file "$ROOT/compiler/limbo/frontend/ALIASES.md" "$destination/ALIASES.md"
        copy_file "$ROOT/compiler/limbo/frontend/ALIASES.ru.md" "$destination/ALIASES.ru.md"
        for header in interface.h signature.h resolve.h native.h source_position.h; do
            copy_file "$ROOT/compiler/limbo/frontend/$header" "$destination/include/$header"
        done
        copy_file "$ROOT/compiler/limbo/frontend/NATIVE_C.md" "$destination/NATIVE_C.md"
        copy_file "$ROOT/compiler/limbo/frontend/NATIVE_C.ru.md" "$destination/NATIVE_C.ru.md"
        sample=Syntax.b
        for tool in limbo_tree limbo_interface limbo_bindings; do
            install_program "$base" "$tool" "$destination"
        done
        copy_file "$ROOT/compiler/limbo/examples/Syntax.b" "$destination/examples/Syntax.b"
        copy_file "$ROOT/compiler/limbo/examples/NativeExample.m" "$destination/examples/NativeExample.m"
        copy_tree "$ROOT/compiler/limbo/examples/external-modules" "$destination/examples/external-modules"
        "${PYTHON:-python3}" "$ROOT/tools/link-installed-limbo-docs.py" "$ROOT" "$destination"
    elif [[ "$language" == as3 ]]; then
        sample=Counter.as
        copy_file "$ROOT/compiler/as3/examples/Counter.as" "$destination/examples/Counter.as"
        copy_file "$ROOT/compiler/as3/examples/Functions.as" "$destination/examples/Functions.as"
    else
        sample=Automatic.as
        copy_file "$ROOT/compiler/$language/examples/Automatic.as" "$destination/examples/Automatic.as"
        copy_file "$ROOT/compiler/$language/examples/UnicodeNames.as" "$destination/examples/UnicodeNames.as"
    fi
    if [[ "$language" == as2 && -d "$base/view" ]]; then
        install_program "$base" as2_viewc "$destination"
        copy_file "$ROOT/compiler/as2/view/view.h" "$destination/include/as2/view.h"
        copy_file "$ROOT/compiler/as2/view/README.md" "$destination/VIEW.md"
        copy_file "$ROOT/compiler/as2/view/README.ru.md" "$destination/VIEW.ru.md"
        copy_file "$ROOT/ide/as/editor-view.as" "$destination/examples/editor-view.as"
    fi
    if [[ "$language" == as1 || "$language" == as2 || "$language" == as3 ]]; then
        copy_tree "$ROOT/compiler/examples/asinc" "$destination/examples/asinc"
        for guide in ASINC.md ASINC.ru.md; do
            sed -e 's|../flashVM/doc/|https://github.com/bhgv/flashVM/blob/master/flashVM/doc/|g' \
                -e 's|../disVM/runtime/|https://github.com/bhgv/flashVM/blob/master/disVM/runtime/|g' \
                "$ROOT/compiler/$guide" >"$destination/$guide"
        done
    fi
    notices "$destination" "$ROOT/compiler/$language"
    copy_file "$ROOT/tools/coco-r-C/upstream/read.me" "$destination/LICENSES/CocoR-README.txt"
    printf 'component=%s\nbuild=%s\nconfiguration=%s\n' "$language" "$base" "$CONFIG" >"$destination/PACKAGE.txt"
    printf 'Installed %s -> %s\n' "$language" "$destination"
    cat >"$destination/README-FIRSTSTEPS.md" <<EOF
# $language syntax frontend

Run from this directory: ./${language}_parse examples/$sample
On Windows append .exe to the executable name.

This is the Coco/R syntax frontend. Acceptance is not proof of full semantic
compilation, FIR lowering or SWF/Dis execution. The optional frontend.h SDK and
static libraries are separate from the command-line examples. Limbo .m scaffolds
remain checked bridge scaffolds, not an already implemented native Dis host.
EOF
}

# Package one requested component with actual products and honest runtime status.
# Parameters: component name. Returns: installation failure propagates to the builder.
# RU: Установить компонент из аргумента; разделяемые helpers наследуют локальные пути.
install_component() {
    local component=$1 base="$BUILD/$1" destination="$OUT/$1" source="$ROOT/$1" profile graphics display variant file sdk_cache native_demo_enabled
    case "$component" in
        compilers)
            for language in as1 as2 as3 limbo; do
                if [[ -d "$base/$language" ]]; then
                    frontend "$language" "$base/$language"
                else
                    frontend "$language" "$BUILD/$language"
                fi
            done
            return 0
            ;;
        as1 | as2 | as3 | limbo)
            frontend "$component" "$base"
            return 0
            ;;
        vm)
            base=${VM_BUILD:-$base}
            profile=$(sed -n 's/^FVM_DEFS_FILE:FILEPATH=//p' "$base/CMakeCache.txt")
            [[ -f "$profile" ]] || fail "No configured FlashVM profile in $base"
            graphics=$(sed -n -E '/^(GRAPHICS_AGG|GRAPHICS_CAIRO|NO_GRAPHICS)[[:space:]]*$/p' "$profile" | head -1)
            display=$(sed -n -E '/^(HEADLESS|FB|GLES|OGL|DX|SDL[123])[[:space:]]*$/p' "$profile" | head -1)
            [[ -n "$graphics" && -n "$display" ]] || fail 'Missing raster/presentation profile keys'
            variant=$(printf '%s-%s' "${graphics#GRAPHICS_}" "$display" | tr '[:upper:]_' '[:lower:]-')
            if grep -q '^FVM_BUILD_DESKTOP_FRONTEND:BOOL=ON$' "$base/CMakeCache.txt"; then
                display=SDL2
                variant=$(printf '%s-sdl2' "${graphics#GRAPHICS_}" | tr '[:upper:]_' '[:lower:]-')
            fi
            if grep -q '^JIT[[:space:]]*$' "$profile"; then
                variant="$variant-jit"
            fi
            destination="$OUT/flashvm/$variant"
            source="$ROOT/flashVM"
            mkdir -p "$destination"
            install_program "$base" flashvm_reference_swf "$destination"
            # Retain an explicit action-only CLI in every package.
            file=$(program "$base" flashvm_reference_swf)
            if [[ "$file" == *.exe ]]; then
                copy_file "$file" "$destination/flashvm-console.exe" 755
            else
                copy_file "$file" "$destination/flashvm-console" 755
            fi
            if [[ "$display" == SDL2 ]]; then
                install_program "$base" flashvm_player "$destination"
                file=$(program "$base" flashvm_player)
                copy_file "$ROOT/flashVM/frontend/run-demo.sh" "$destination/run-demo.sh" 755
                copy_file "$ROOT/external/sdl2/LICENSE.txt" "$destination/LICENSES/SDL2.txt"
                sdk_cache="$BUILD/deps/sdl2/sdk"
                if [[ -d "$sdk_cache/licenses" ]]; then
                    copy_tree "$sdk_cache/licenses" "$destination/LICENSES/desktop-sdk"
                fi
                if [[ -f "$sdk_cache/CAPABILITIES.txt" ]]; then
                    copy_file "$sdk_cache/CAPABILITIES.txt" "$destination/BUILD-CAPABILITIES.txt"
                fi
                copy_file "$ROOT/tools/sdl2/run-packaged-demo.sh" "$OUT/flashvm/run-demo.sh" 755
            fi
            # flashvm is the real windowed player for an SDL package, console otherwise.
            if [[ "$file" == *.exe ]]; then
                copy_file "$file" "$destination/flashvm.exe" 755
            else
                copy_file "$file" "$destination/flashvm" 755
            fi
            copy_tree "$ROOT/flashVM/include/flashvm" "$destination/include/flashvm"
            copy_file "$profile" "$destination/configured.defs"
            for example in constant arithmetic; do
                copy_file "$ROOT/examples/firststeps/$example.swf" "$destination/$example.swf"
                copy_file "$ROOT/examples/firststeps/$example.as" "$destination/$example.as"
            done
            copy_tree "$ROOT/flashVM/examples/player/animation_audio" "$destination/examples/player/animation_audio"
            copy_tree "$ROOT/flashVM/examples/player/wheel_cursor" "$destination/examples/player/wheel_cursor"
            copy_tree "$ROOT/flashVM/examples/player/frame_capture" "$destination/examples/player/frame_capture"
            copy_tree "$ROOT/examples/firststeps/video" "$destination/examples/video"
            cat >"$destination/README-FIRSTSTEPS.md" <<EOF
# FlashVM: $graphics / $display

Console example: ./flashvm-console constant.swf   (expected trace: 123)
Console example: ./flashvm-console arithmetic.swf (expected trace: 42)
Windows: use flashvm.exe. The corresponding commented AS sources are alongside.

With display=SDL2, flashvm is the real native player: use ./run-demo.sh or
./flashvm examples/player/animation_audio/02-input-audio.swf. The separate
flashvm-console command is the bounded action-only runner.
The configured raster and presentation keys are $graphics / $display.
HEADLESS does not create a native window. Other variants occupy separate folders;
copying a runner cannot add a presentation backend that the VM does not implement.
Static SDK libraries and public headers are provided for host embedding.

## Graphics, input and sound

With the optional native player installed, run:
./flashvm_player --no-jit --stats examples/player/animation_audio/02-input-audio.swf

Arrows move the blue box; Space stretches it; the pink marker follows the mouse;
clicks move the yellow marker. Click the window to give keyboard focus. The movie
also exercises PCM/ADPCM event sounds. 01 is animation/audio without required
input; 03 checks StopSounds. --mute allows visual/input checks without sound output.
The fully commented input_equivalent.as and audio-timeline.txt explain the actions.
wheel_cursor demonstrates wheel/cursor events; frame_capture demonstrates drawing.

## Video compatibility fixtures

examples/video contains genuine SWF6 Sorenson Spark video, including a video+MP3
variant. Current FlashVM does not implement DefineVideoStream/VideoFrame playback.
Read examples/video/README.md; a successful load or blank window is not video acceptance.
EOF
            ;;
        fir)
            install_fir_package
            ;;
        linker)
            mkdir -p "$destination/examples"
            install_program "$base" swl "$destination"
            copy_tree "$ROOT/linker/include" "$destination/include"
            copy_tree "$ROOT/examples/firststeps" "$destination/examples/firststeps"
            copy_tree "$ROOT/linker/examples/inspect" "$destination/examples/inspect"
            cat >"$destination/README-FIRSTSTEPS.md" <<'EOF'
# SWL linker

./swl link arithmetic.swf ../fir/arithmetic.swo

First create that object using the FIR package instructions. The supplied .as,
.fir and .swf files are matched examples; the inspect directory contains a C99
public-API client. This package does not turn arbitrary AS into complete SWF.
EOF
            ;;
        symbols)
            source="$ROOT/compiler/limbo/symbols"
            mkdir -p "$destination/examples"
            install_program "$base" sblmap "$destination"
            install_program "$base" sblquery "$destination"
            copy_tree "$source/include" "$destination/include"
            if [[ -f "$BUILD/ide/idecore.sbl" ]]; then
                copy_file "$BUILD/ide/idecore.sbl" "$destination/examples/idecore.sbl"
            fi
            printf '%s\n' '# Limbo symbols' './sblmap examples/idecore.sbl' 'Build IDE first to include its real .sbl example.' >"$destination/README-FIRSTSTEPS.md"
            ;;
        disvm-runtime | disvm-editor)
            install_runtime_package
            ;;
        disvm | disvm-host)
            install_host_package
            ;;
        ide)
            source="$ROOT/ide/dis"
            mkdir -p "$destination/modules" "$destination/vm"
            copy_file "$base/idecore.dis" "$destination/modules/idecore.dis"
            copy_file "$base/idecore.sbl" "$destination/modules/idecore.sbl"
            copy_file "$source/idecore.m" "$destination/include/idecore.m"
            copy_tree "$source/examples" "$destination/examples"
            copy_tree "$source/bridge" "$destination/include/bridge"
            copy_tree "$ROOT/ide/as" "$destination/as"
            copy_file "$ROOT/disVM/examples/ui-channel/bridge.as" "$destination/as/bridge.as"
            copy_file "$ROOT/disVM/OWNER-PUMP.ru.md" "$destination/OWNER-PUMP.ru.md"
            copy_file "$ROOT/disVM/module/uibridge.m" "$destination/include/uibridge.m"
            copy_file "$ROOT/ide/EDITOR-CHANNELS.ru.md" "$destination/EDITOR-CHANNELS.ru.md"
            copy_file "$ROOT/ide/WORKSPACE.md" "$destination/WORKSPACE.md"
            for file in uibridge ideinput idebridge idecomplete idebuiltins ideworkspace idefiles workspace-example navigation history replaceall idevalue ideprojection ideinspect idedocprojection; do
                if [[ -f "$base/$file.dis" ]]; then
                    copy_file "$base/$file.dis" "$destination/modules/$file.dis"
                    if [[ -f "$base/$file.sbl" ]]; then
                        copy_file "$base/$file.sbl" "$destination/modules/$file.sbl"
                    fi
                fi
            done
            if [[ -n "${EMU_EXECUTABLE:-}" && -f "${EMU_EXECUTABLE:-}" ]]; then
                copy_file "$EMU_EXECUTABLE" "$destination/vm/emu" 755
                printf '%s\n' 'Explicit reference emu copied; its matching module namespace is still required.' >"$destination/vm/STATUS.txt"
            else
                printf '%s\n' 'These modules use the classic Dis ABI; disvm_core is a separate SDK.' 'Select disvm-runtime (included in all) for matching NC64 modules and ../disvm-runtime/run-ide.sh.' >"$destination/vm/STATUS.txt"
            fi
            printf '%s\n' '# IDE document-service package' 'Classic modules/debug symbols are compiled. The separate disvm-runtime package runs matching NC64 IDE services; graphical editor integration remains unfinished.' 'See vm/STATUS.txt. A FlashVM SWF runner cannot execute idecore.dis.' 'No fake runtime or silent emu dependency is installed.' >"$destination/README-FIRSTSTEPS.md"
            ;;
        mk | iyacc | limbo-bootstrap | cocor | cocor-limbo)
            case "$component" in
                mk)
                    source="$ROOT/tools/mk"
                    file=mk
                    ;;
                iyacc)
                    source="$ROOT/tools/iyacc"
                    file=iyacc
                    ;;
                limbo-bootstrap)
                    source="$ROOT/tools/limbo-bootstrap"
                    file=limbo-bootstrap
                    ;;
                cocor)
                    source="$ROOT/tools/coco-r-C"
                    file=cocor
                    ;;
                cocor-limbo)
                    source="$ROOT/tools/coco-r-limbo"
                    file=cocor
                    ;;
            esac
            mkdir -p "$destination"
            install_program "$base" "$file" "$destination"
            if [[ "$component" == cocor || "$component" == cocor-limbo ]]; then
                copy_tree "$source/upstream/frames" "$destination/frames"
                copy_tree "$source/upstream/sources" "$destination/sources"
                copy_file "$source/upstream/read.me" "$destination/UPSTREAM-README.txt"
                copy_tree "$ROOT/examples/firststeps/tools/cocor" "$destination/examples"
                if [[ -f "$base/cocor.dis" ]]; then
                    copy_file "$base/cocor.dis" "$destination/cocor.dis"
                fi
            elif [[ "$component" == iyacc ]]; then
                copy_file "$source/upstream/utils/iyacc/yaccpar" "$destination/utils/lib/yaccpar"
                copy_tree "$ROOT/examples/firststeps/tools/iyacc" "$destination/examples"
            elif [[ "$component" == mk ]]; then
                copy_tree "$ROOT/examples/firststeps/tools/mk" "$destination/examples"
            elif [[ "$component" == limbo-bootstrap ]]; then
                copy_tree "$source/upstream/module" "$destination/module"
                copy_tree "$ROOT/examples/firststeps/tools/limbo-scalar" "$destination/examples/dis64-scalar"
                copy_file "$source/upstream/limbo/tests/README.dis64-scalar.md" "$destination/examples/dis64-scalar/UPSTREAM-SCALAR.md"
                copy_file "$ROOT/compiler/limbo/examples/external-modules/arithmetic.b" "$destination/examples/arithmetic.b"
                copy_file "$ROOT/compiler/limbo/examples/external-modules/arithmetic.m" "$destination/examples/arithmetic.m"
            fi
            copy_file "$ROOT/examples/firststeps/tools/README.md" "$destination/EXAMPLES.md"
            printf '# %s\n\nSource-built bootstrap/development tool; see preserved notices and UPSTREAM.json.\n' "$component" >"$destination/README-FIRSTSTEPS.md"
            ;;
        *)
            fail "Unsupported package: $component"
            ;;
    esac
    libraries "$base" "$destination"
    if [[ "$component" == disvm-runtime || "$component" == disvm-editor ]]; then
        # Retire this obsolete installer-owned location after copying libraries.
        rm -f -- "$destination/lib/libdisvm_compiler_engine.a"
        if [[ "$compiler_enabled" != ON ]]; then
            rm -f -- "$destination/lib/compiler/libdisvm_compiler_engine.a" "$destination/buildpanel-test.dis" "$destination/buildpanel-test.sbl" "$destination/build-panel.dis" "$destination/build-panel.sbl" "$destination/artifact-export.dis" "$destination/artifact-export.sbl" "$destination/artifact-test.dis" "$destination/artifact-test.sbl"
        fi
    fi
    notices "$destination" "$source"
    printf 'component=%s\nbuild=%s\nconfiguration=%s\n' "$component" "$base" "$CONFIG" >"$destination/PACKAGE.txt"
    printf 'Installed %s -> %s\n' "$component" "$destination"
}

while (($#)); do
    (($# >= 2)) || fail "Missing value for $1"
    case "$1" in
        --projects)
            PROJECTS=$2
            ;;
        --build-root)
            BUILD=$2
            ;;
        --out)
            OUT=$2
            ;;
        --config)
            CONFIG=$2
            ;;
        --vm-build)
            VM_BUILD=$2
            ;;
        *)
            fail "Unknown argument: $1"
            ;;
    esac
    shift 2
done
[[ -n "$PROJECTS" && -n "$BUILD" && -n "$OUT" ]] || fail 'Provide nonempty projects/build/out'
IFS=, read -r -a selected <<<"$PROJECTS"
for component in "${selected[@]}"; do
    install_component "$component"
done

# Install one discoverable entry point after all requested packages succeed.
copy_file "$ROOT/tools/run-installed-checks.sh" "$OUT/run-checks.sh" 755
cat >"$OUT/README-RUN.txt" <<'GUIDE'
Run ./run-checks.sh for installed FlashVM/Dis native checks.
Run ./run-checks.sh --window for the actual window, input and sound demo.
The graphical player accepts --frames N, --mute, --stats and --no-jit.
Interactive Dis editor: sh disvm-runtime/run-editor.sh (or menu item 5).
Each selected component has its own README, examples and actual products.
Core Dis/JIT diagnostics are separate from the experimental NC64 application runtime.
The terminal IDE runs in disvm-runtime; graphical-editor integration remains in progress.
GUIDE
