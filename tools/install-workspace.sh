#!/usr/bin/env bash
# Install actual workspace products and owned examples without requiring CMake at install time.
set -euo pipefail
ROOT=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd) # Workspace source root.
BUILD="$ROOT/build/workspace"                                       # Existing build tree, never cleaned here.
OUT="$ROOT/out"                                                     # User-facing local installation prefix.
PROJECTS=""                                                         # Explicit comma-separated package selection.
CONFIG=Release                                                      # Multi-configuration build directory spelling.
VM_BUILD=""                                                         # Optional separately built FlashVM variant.

# Reject an incomplete package instead of silently reporting a successful installation.
# Parameters: diagnostic words. Returns: never; writes stderr and exits with status 1.
fail() {
    printf 'install-workspace: %s\n' "$*" >&2
    exit 1
}

# Copy one known product, retaining ordinary data permissions or setting executable mode.
# Parameters: source, destination, optional mode. Returns: cp/chmod failure propagates.
copy_file() {
    local source=$1 destination=$2 mode=${3:-644}
    [[ -f "$source" ]] || fail "Missing product: $source"
    mkdir -p "$(dirname -- "$destination")"
    cp -p -- "$source" "$destination"
    chmod "$mode" "$destination"
}

# Copy an explicitly selected SDK/example directory; no source checkout is removed.
# Parameters: source directory and destination directory. Returns: copy failure propagates.
copy_tree() {
    local source=$1 destination=$2
    [[ -d "$source" ]] || fail "Missing SDK directory: $source"
    mkdir -p "$destination"
    cp -R -- "$source/." "$destination/"
}

# Locate a named executable in native, CMake or multi-configuration output layouts.
# Parameters: build directory and target basename. Returns: prints a path, or fails visibly.
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
install_program() {
    local source name=$2 destination=$3
    source=$(program "$1" "$name")
    if [[ "$source" == *.exe ]]; then
        name="$name.exe"
    fi
    copy_file "$source" "$destination/$name" 755
}

# Preserve first-party licensing plus selected upstream notices rather than relabeling code.
# Parameters: package directory and component source directory. Returns: copy failure propagates.
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
        for header in interface.h signature.h native.h; do
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
            source="$ROOT/fir/v1"
            mkdir -p "$destination/examples"
            for file in fir1 fir1_source fir1_opt fir1_table_run; do
                install_program "$base" "$file" "$destination"
            done
            install_program "$base" fir1_dis_example "$destination"
            copy_tree "$ROOT/examples/firststeps" "$destination/examples/firststeps"
            copy_tree "$source/examples/dis" "$destination/examples/dis"
            copy_tree "$source/examples/tables" "$destination/examples/tables"
            copy_tree "$source/include" "$destination/include"
            copy_file "$source/compiler/dis.c" "$destination/source/dis.c"
            copy_file "$source/compiler/dis_table.c" "$destination/source/dis_table.c"
            for file in DIS_BACKEND.md DIS_BACKEND.ru.md TABLES.md TABLES.ru.md TABLES_DIS.md TABLES_DIS.ru.md EXTENSIONS.md EXTENSIONS.ru.md TYPE_REFLECTION.md TYPE_REFLECTION.ru.md BIG_ARITHMETIC.md BIG_ARITHMETIC.ru.md BIG_BITWISE.md BIG_BITWISE.ru.md BIG_CFG.md BIG_CFG.ru.md BIG_DECISIONS.md BIG_DECISIONS.ru.md TABLE_CFG.md TABLE_CFG.ru.md TABLE_MUTATION.md TABLE_MUTATION.ru.md TERMINAL_ARMS.md TERMINAL_ARMS.ru.md COMPARISON_VALUES.md COMPARISON_VALUES.ru.md SHORT_CIRCUIT.md SHORT_CIRCUIT.ru.md COMPOUND_UPDATES.md COMPOUND_UPDATES.ru.md UNICODE_STRINGS.md UNICODE_STRINGS.ru.md INT_ARITHMETIC.md INT_ARITHMETIC.ru.md TEXT_BITS.md TEXT_BITS.ru.md INT_SHIFTS.md INT_SHIFTS.ru.md TEXT_VALUES.md TEXT_VALUES.ru.md TABLE_LITERALS.md TABLE_LITERALS.ru.md RELATIVE_SEQUENCES.md RELATIVE_SEQUENCES.ru.md SLICE_ASSIGNMENT.md SLICE_ASSIGNMENT.ru.md BIG_SHIFTS.md BIG_SHIFTS.ru.md; do
                copy_file "$source/doc/$file" "$destination/docs/$file"
            done
            for file in API.md API.ru.md; do
                copy_file "$source/compiler/$file" "$destination/docs/$file"
            done
            "${PYTHON:-python3}" "$ROOT/tools/link-installed-fir-docs.py" "$ROOT" "$destination"
            cat >"$destination/README-FIRSTSTEPS.md" <<'EOF'
# Numeric source -> FIR -> object

./fir1_source as2 --input examples/firststeps/arithmetic.as arithmetic.fir
./fir1 check arithmetic.fir
./fir1 opt arithmetic.fir arithmetic.opt.fir
./fir1 object arithmetic.opt.fir arithmetic.swo

Pass arithmetic.swo to the separately packaged swl linker. The included source,
expected FIR and SWF illustrate the numeric subset, not a complete AS compiler.
If your host restricts temporary files, set TMPDIR to an existing writable folder.

Typed tables: [source/FIR](docs/TABLES.md), [native Dis](docs/TABLES_DIS.md),
[русский контракт](docs/TABLES_DIS.ru.md). Compile examples/tables/main.b with
fir1_source, then fir1 dis tables.fir tables.dis and run it with the matching
hosted disvm --jit --quantum 1 --inspect-words 0 2 tables.dis. Expected: 42, 1.
The reference runner and C99 reference/compiler clients remain available separately.

# Our Limbo numeric subset -> direct NC64 Dis (no limbo-native/assembler)
./fir1_source limbo --input examples/dis/main.b answer.fir
./fir1 dis answer.fir answer.dis
# Run with the separately installed NC64 DisVM:
../disvm-editor/disvm --jit --inspect-words 0 1 answer.dis
# Expected: DATA 0 2147483647

Read examples/dis/README.md and README.ru.md, plus docs/DIS_BACKEND.md/.ru.md.
The target captures i32/bool/f64 trace values plus integer executed flags after completion.
It supports verified CFG and terminal Limbo decisions, not full module lowering. The C99 example and libfir1_ir/libfir1_source_api/libfir1_io are included.

# Terminal decision: values [0,42,0], executed flags [0,1,0].
./fir1_source limbo --input examples/dis/decision.b decision.fir
./fir1 dis decision.fir decision.dis
../disvm-editor/disvm --jit --max-instructions 1000 --inspect-words 0 6 decision.dis

# Numeric AS3 binary64: -11.5, 3.75, -1.25; flags at offsets 24/32/40.
./fir1_source as3 --input examples/dis/reals.as reals.fir
./fir1 dis reals.fir reals.dis
../disvm-editor/disvm --jit --inspect-reals 0 3 reals.dis

# Homogeneous Limbo real: explicit real return type, result 42.5.
./fir1_source limbo --input examples/dis/real.b real.fir
./fir1 dis real.fir real.dis
../disvm-editor/disvm --jit --inspect-reals 0 1 real.dis
EOF
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
            source="$ROOT/disVM"
            mkdir -p "$destination"
            copy_tree "$ROOT/disVM/examples/native-module" "$destination/examples/native-module"
            copy_file "$ROOT/disVM/runtime/NATIVE_MODULES.md" "$destination/NATIVE_MODULES.md"
            copy_file "$ROOT/disVM/runtime/NATIVE_MODULES.ru.md" "$destination/NATIVE_MODULES.ru.md"
            copy_file "$ROOT/compiler/limbo/frontend/NATIVE_C.md" "$destination/docs/limbo/NATIVE_C.md"
            copy_file "$ROOT/compiler/limbo/frontend/NATIVE_C.ru.md" "$destination/docs/limbo/NATIVE_C.ru.md"
            copy_file "$ROOT/flashVM/doc/RENDER_ASYNC.md" "$destination/docs/ide/RENDER_ASYNC.md"
            copy_file "$ROOT/flashVM/doc/RENDER_ASYNC.ru.md" "$destination/docs/ide/RENDER_ASYNC.ru.md"
            copy_file "$ROOT/ide/RENDER_OWNERS.md" "$destination/docs/ide/RENDER_OWNERS.md"
            copy_file "$ROOT/ide/RENDER_OWNERS.ru.md" "$destination/docs/ide/RENDER_OWNERS.ru.md"
            copy_file "$ROOT/disVM/examples/native-module/nativedemo.m" "$destination/include/limbo/nativedemo.m"
            native_demo_enabled=OFF
            if [[ -f "$base/CMakeCache.txt" ]] && grep -q '^DVM_RUNTIME_NATIVE_DEMO:BOOL=ON$' "$base/CMakeCache.txt"; then
                native_demo_enabled=ON
            fi
            printf 'Nativedemo=%s\n' "$native_demo_enabled" >"$destination/NATIVE-MODULES.txt"
            if [[ "$native_demo_enabled" == ON ]]; then
                copy_file "$base/native/nativedemo_native.h" "$destination/examples/native-module/nativedemo_native.h"
            else
                # Retire only these installer-owned products when an existing package is switched off.
                rm -f -- "$destination/native-demo.dis" "$destination/native-demo.sbl" "$destination/examples/native-module/nativedemo_native.h"
            fi
            install_program "$base" disvm "$destination"
            if [[ "$component" == disvm-editor ]]; then
                install_program "$base" disvm-editor "$destination"
                copy_tree "$ROOT/ide/as" "$destination/source/as"
                copy_file "$ROOT/compiler/as2/view/README.md" "$destination/docs/as2/VIEW.md"
                copy_file "$ROOT/compiler/as2/view/README.ru.md" "$destination/docs/as2/VIEW.ru.md"
                copy_file "$ROOT/fir/v1/doc/AS_PROPERTIES.md" "$destination/docs/fir/AS_PROPERTIES.md"
                copy_file "$ROOT/fir/v1/doc/AS_PROPERTIES.ru.md" "$destination/docs/fir/AS_PROPERTIES.ru.md"
                for artifact in fir opt.fir swo swf; do
                    if [[ -f "$base/native/ide-view.$artifact" ]]; then
                        copy_file "$base/native/ide-view.$artifact" "$destination/ui/editor-view.$artifact"
                    fi
                done
                copy_tree "$ROOT/ide/assets/fonts" "$destination/fonts"
                for file in "$ROOT/ide/dis/"*.md; do
                    copy_file "$file" "$destination/docs/dis/$(basename "$file")"
                done
                copy_file "$ROOT/ide/assets/fonts/OFL.txt" "$destination/LICENSES/Noto-OFL.txt"
                copy_file "$ROOT/external/sdl2/LICENSE.txt" "$destination/LICENSES/SDL2.txt"
                copy_file "$ROOT/disVM/runtime/run-graphical-editor.sh" "$destination/run-graphical-editor.sh" 755
                copy_tree "$ROOT/ide/dis/app" "$destination/source/app"
                copy_tree "$ROOT/ide/dis/examples/graphical-editor" "$destination/examples/graphical-editor"
                copy_file "$ROOT/ide/dis/bridge/idehost.m" "$destination/include/limbo/idehost.m"
            fi
            file=$(program "$base" limbo-bootstrap)
            copy_file "$file" "$destination/limbo-native" 755
            copy_file "$base/bootstrap/target-abi.json" "$destination/ABI.json"
            for file in "$base"/*.dis "$base"/*.sbl; do
                if [[ "$native_demo_enabled" != ON && ( "$(basename "$file")" == native-demo.dis || "$(basename "$file")" == native-demo.sbl ) ]]; then
                    continue
                fi
                if [[ -f "$file" ]]; then
                    copy_file "$file" "$destination/$(basename "$file")"
                fi
            done
            copy_file "$ROOT/disVM/runtime/run-console.sh" "$destination/run-console.sh" 755
            if [[ "$component" == disvm-editor ]]; then
                copy_file "$ROOT/disVM/runtime/run-ide-window.sh" "$destination/run-ide.sh" 755
                copy_file "$ROOT/disVM/runtime/run-ide.sh" "$destination/run-ide-console.sh" 755
            else
                copy_file "$ROOT/disVM/runtime/run-ide.sh" "$destination/run-ide.sh" 755
            fi
            copy_file "$ROOT/disVM/runtime/run-editor.sh" "$destination/run-editor.sh" 755
            copy_file "$ROOT/disVM/runtime/run-modules.sh" "$destination/run-modules.sh" 755
            copy_tree "$ROOT/disVM/examples/standard-modules" "$destination/examples/standard-modules"
            copy_tree "$ROOT/disVM/examples/decimal-append" "$destination/examples/decimal-append"
            copy_tree "$ROOT/ide/dis/examples/directory-browser" "$destination/examples/directory-browser"
            copy_file "$ROOT/disVM/runtime/test_directory_browser.py" "$destination/examples/directory-browser/run.py"
            copy_file "$ROOT/ide/dis/GRAPHICAL-BROWSER.md" "$destination/GRAPHICAL-BROWSER.md"
            copy_file "$ROOT/ide/dis/GRAPHICAL-BROWSER.ru.md" "$destination/GRAPHICAL-BROWSER.ru.md"
            copy_file "$ROOT/disVM/runtime/test_graphical_browser.py" "$destination/examples/directory-browser/graphical-check.py"
            copy_file "$ROOT/disVM/runtime/test_graphical_editor.py" "$destination/examples/directory-browser/test_graphical_editor.py"
            copy_file "$ROOT/ide/dis/DIRECTORIES.md" "$destination/DIRECTORIES.md"
            copy_file "$ROOT/ide/dis/DIRECTORIES.ru.md" "$destination/DIRECTORIES.ru.md"
            copy_tree "$ROOT/disVM/examples/filesystem" "$destination/examples/filesystem"
            copy_file "$ROOT/disVM/runtime/test_filesystem.py" "$destination/examples/filesystem/run.py"
            copy_file "$ROOT/disVM/runtime/FILESYSTEM.md" "$destination/FILESYSTEM.md"
            copy_file "$ROOT/disVM/runtime/FILESYSTEM.ru.md" "$destination/FILESYSTEM.ru.md"
            copy_tree "$ROOT/disVM/examples/relative-sequences" "$destination/examples/relative-sequences"
            copy_tree "$ROOT/disVM/examples/hybrid-table" "$destination/examples/hybrid-table"
            copy_tree "$ROOT/disVM/examples/table-guards" "$destination/examples/table-guards"
            copy_tree "$ROOT/disVM/examples/table-leases" "$destination/examples/table-leases"
            copy_tree "$ROOT/common/owned" "$destination/source/owned"
            copy_tree "$ROOT/common/table" "$destination/source/table"
            copy_file "$ROOT/LICENSE" "$destination/source/owned/LICENSE"
            copy_file "$ROOT/LICENSE" "$destination/source/table/LICENSE"
            copy_file "$ROOT/disVM/PAYLOAD_GUARDS.md" "$destination/PAYLOAD_GUARDS.md"
            copy_file "$ROOT/disVM/PAYLOAD_GUARDS.ru.md" "$destination/PAYLOAD_GUARDS.ru.md"
            copy_tree "$ROOT/common/table" "$destination/source/hybrid-table-core"
            copy_file "$ROOT/LICENSE" "$destination/source/hybrid-table-core/LICENSE"
            copy_file "$ROOT/disVM/libraries/README.md" "$destination/STANDARD-MODULES.md"
            copy_file "$ROOT/disVM/MODULE_AUDIT.md" "$destination/MODULE_AUDIT.md"
            copy_file "$ROOT/disVM/MODULE_AUDIT.ru.md" "$destination/MODULE_AUDIT.ru.md"
            for file in "$base/native/libraries/"*.b; do
                copy_file "$file" "$destination/source/libraries/$(basename "$file")"
            done
            copy_tree "$ROOT/ide/dis/examples/editor-console" "$destination/examples/editor-console"
            copy_file "$ROOT/disVM/runtime/README.md" "$destination/README-FIRSTSTEPS.md"
            "${PYTHON:-python3}" "$ROOT/tools/link-installed-fir-docs.py" "$ROOT" "$destination" --runtime
            copy_tree "$ROOT/ide/dis/examples/native-console" "$destination/examples/native-console"
            copy_tree "$ROOT/disVM/examples/console-check" "$destination/examples/console-check"
            copy_tree "$ROOT/ide/dis/examples/workspace" "$destination/examples/workspace"
            copy_tree "$ROOT/tools/limbo-bootstrap/upstream/module" "$destination/include/limbo"
            copy_file "$ROOT/disVM/upstream/module/readdir.m" "$destination/include/limbo/readdir.m"
            copy_file "$ROOT/disVM/upstream/module/regex.m" "$destination/include/limbo/regex.m"
            copy_file "$ROOT/disVM/upstream/appl/lib/tests/regex-unicode.b" "$destination/examples/regex-unicode.b"
            copy_file "$ROOT/ide/dis/idecore.m" "$destination/include/limbo/idecore.m"
            copy_file "$ROOT/disVM/module/uibridge.m" "$destination/include/limbo/uibridge.m"
            copy_file "$ROOT/disVM/module/bigtable.m" "$destination/include/limbo/bigtable.m"
            copy_file "$ROOT/disVM/broker/brokerdiag.m" "$destination/include/limbo/brokerdiag.m"
            for file in broker.h codec.h flash_channel.h; do
                copy_file "$ROOT/disVM/broker/$file" "$destination/include/disvm/$file"
            done
            copy_tree "$ROOT/disVM/broker" "$destination/examples/broker"
            copy_tree "$ROOT/ide/dis/examples/projection" "$destination/examples/projection"
            copy_tree "$ROOT/ide/dis/examples/document-projection" "$destination/examples/document-projection"
            copy_file "$ROOT/ide/as/projection-values.as" "$destination/as/projection-values.as"
            for file in idevalue ideprojection ideinspect idedocprojection idebrowser; do
                copy_file "$ROOT/ide/dis/bridge/$file.b" "$destination/source/$file.b"
            done
            copy_file "$ROOT/ide/dis/PROJECTIONS.md" "$destination/PROJECTIONS.md"
            copy_file "$ROOT/ide/dis/VALUES.md" "$destination/VALUES.md"
            copy_file "$ROOT/ide/dis/VALUES.ru.md" "$destination/VALUES.ru.md"
            copy_file "$ROOT/ide/dis/LIVE-PROJECTIONS.md" "$destination/LIVE-PROJECTIONS.md"
            copy_file "$ROOT/ide/dis/LIVE-PROJECTIONS.ru.md" "$destination/LIVE-PROJECTIONS.ru.md"
            copy_file "$ROOT/ide/dis/WINDOWS-GUTTER.md" "$destination/WINDOWS-GUTTER.md"
            copy_file "$ROOT/ide/dis/WINDOWS-GUTTER.ru.md" "$destination/WINDOWS-GUTTER.ru.md"
            copy_file "$ROOT/ide/dis/bridge/idefiles.README.md" "$destination/FILE-WINDOWS.md"
            copy_file "$ROOT/ide/dis/bridge/idefiles.b" "$destination/source/idefiles.b"
            for file in "$ROOT/ide/dis/bridge/"*.m; do
                copy_file "$file" "$destination/include/limbo/$(basename "$file")"
            done
            "${PYTHON:-python3}" "$ROOT/tools/link-installed-runtime-docs.py" "$ROOT" "$destination"
            ;;
        disvm | disvm-host)
            source="$ROOT/disVM"
            mkdir -p "$destination"
            copy_tree "$ROOT/disVM/include" "$destination/include"
            # Export the public header closure required by flashvm_host.h.
            copy_tree "$ROOT/flashVM/include" "$destination/include"
            for file in thread_port.h channel_sync.h file_port.h filesystem_port.h network/socket.h network/resolver.h; do
                copy_file "$ROOT/flashVM/ports/$file" "$destination/include/$file"
            done
            copy_tree "$ROOT/disVM/examples/module-image" "$destination/examples/module-image"
            copy_tree "$ROOT/disVM/examples/module-store" "$destination/examples/module-store"
            copy_tree "$ROOT/disVM/examples/console-check" "$destination/examples/console-check"
            if [[ -f "$base/disvm-check" && -f "$base/disvm-jit-word32" && -f "$base/disvm-jit-word64" ]] &&
                grep -q '^DVM_BUILD_DIAGNOSTICS:BOOL=ON$' "$base/CMakeCache.txt"; then
                for file in disvm-check disvm-jit-word32 disvm-jit-word64; do
                    install_program "$base" "$file" "$destination"
                done
            fi
            for file in console-check.dis console-check.sbl; do
                if [[ -f "$base/$file" ]]; then
                    copy_file "$base/$file" "$destination/$file"
                fi
            done
            if [[ -n "$(find "$base" -type f \( -name disvm_module_store_example -o -name disvm_module_store_example.exe \) -print -quit)" ]]; then
                install_program "$base" disvm_module_store_example "$destination"
            fi
            copy_file "$ROOT/disVM/MODULE-STORE.md" "$destination/MODULE-STORE.md"
            copy_file "$ROOT/disVM/RUNNABLE-CHECKS.md" "$destination/RUNNABLE-CHECKS.md"
            copy_tree "$ROOT/disVM/module" "$destination/module"
            copy_tree "$ROOT/disVM/limbo" "$destination/limbo"
            copy_tree "$ROOT/disVM/examples/ui-channel" "$destination/examples/ui-channel"
            copy_file "$ROOT/disVM/OWNER-PUMP.ru.md" "$destination/OWNER-PUMP.ru.md"
            printf '%s\n' '# Dis native SDK' 'Run ./disvm-check when native AMD64 diagnostics were built. This executes real Dis/JIT checks without external emu.' 'console-check.dis is a genuine Limbo channel/Sys test program; its full embedded module host is still in progress.' 'The native module/scheduler/GC host integration remains in progress.' >"$destination/README-FIRSTSTEPS.md"
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
