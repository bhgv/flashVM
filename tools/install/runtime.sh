# Copyright (c) 2026 bhgv
# EN: Runtime/editor documentation, headers and portable products.
# RU: Упаковка продуктов без исполнения целевых программ.


# EN: Borrow install_component locals (base/destination/component/source) through Bash scope;
# use shared copy/program helpers. Copy failures propagate through set -e; no owned inputs.
# RU: Заимствовать base/destination/component/source из install_component через область Bash;
# использовать общие copy/program. Ошибки передаются через set -e, входы не поглощаются.
install_runtime_package() {
    source="$ROOT/disVM"
    install_dis64_envelope "$base" "$destination"
    mkdir -p "$destination"
    copy_tree "$ROOT/disVM/examples/native-module" "$destination/examples/native-module"
    copy_tree "$ROOT/flashVM/examples/embedding/native_module" "$destination/examples/flashvm-native-module"
    if [[ -f "$base/disvm_flashvm_native_module_example" && -f "$base/native/native-module.swf" ]]; then
        install_program "$base" disvm_flashvm_native_module_example "$destination"
        copy_file "$ROOT/disVM/runtime/run-flashvm-native-module.sh" "$destination/run-flashvm-native-module.sh" 755
        copy_file "$base/native/native-module.swf" "$destination/examples/flashvm-native-module/native-module.swf"
    fi
    copy_file "$ROOT/disVM/runtime/FLASHVM-NATIVE-MODULE.md" "$destination/examples/flashvm-native-module/README.md"
    mkdir -p "$destination/docs/ide"
    for language in md ru.md; do
        sed -e 's|../../flashVM/|https://github.com/bhgv/flashVM/blob/master/flashVM/|g' \
            -e 's|../../disVM/|https://github.com/bhgv/flashVM/blob/master/disVM/|g' \
            "$ROOT/ide/dis/DESKTOP-CONTROLS.$language" >"$destination/docs/ide/DESKTOP-CONTROLS.$language"
    done
    copy_file "$ROOT/disVM/NATIVE64-REVIEW.md" "$destination/docs/dis64/NATIVE64-REVIEW.md"
    copy_file "$ROOT/disVM/NATIVE64-REVIEW.ru.md" "$destination/docs/dis64/NATIVE64-REVIEW.ru.md"
    copy_file "$ROOT/disVM/runtime/RETAINED-SCENE.md" "$destination/RETAINED-SCENE.md"
    copy_file "$ROOT/disVM/runtime/RETAINED-SCENE.ru.md" "$destination/RETAINED-SCENE.ru.md"
    copy_file "$ROOT/disVM/runtime/SYS_CAPTURE.md" "$destination/SYS_CAPTURE.md"
    copy_file "$ROOT/disVM/runtime/SYS_CAPTURE.ru.md" "$destination/SYS_CAPTURE.ru.md"
    copy_file "$ROOT/disVM/runtime/NATIVE_MODULES.md" "$destination/NATIVE_MODULES.md"
    copy_file "$ROOT/disVM/runtime/NATIVE_MODULES.ru.md" "$destination/NATIVE_MODULES.ru.md"
    copy_file "$ROOT/ide/BULK-GEOMETRY.md" "$destination/BULK-GEOMETRY.md"
    copy_file "$ROOT/ide/BULK-GEOMETRY.ru.md" "$destination/BULK-GEOMETRY.ru.md"
    copy_file "$ROOT/ide/NATIVE-INK.md" "$destination/NATIVE-INK.md"
    copy_file "$ROOT/ide/NATIVE-INK.ru.md" "$destination/NATIVE-INK.ru.md"
    for native_ink_file in editor_primitive.h editor_xor.h editor_vector_layout.h editor_full_raster.h editor_ink.c editor_ink.h editor_ink_pointer.h editor_wheel_batch.h editor_bulk.c editor_bulk.h editor_motion_batch.c editor_motion_batch.h editor_stamp.c editor_stamp.h editor_host.c; do
        copy_file "$ROOT/disVM/runtime/$native_ink_file" "$destination/source/native-ink/$native_ink_file"
    done
    for chrome_file in desktop_round_primitive.h desktop_chrome.h desktop_drawing_capability.c desktop_paint_primitives.c desktop_window_chrome.c; do
        copy_file "$ROOT/disVM/runtime/$chrome_file" "$destination/source/chrome-native/$chrome_file"
    done
    copy_tree "$ROOT/ide/dis/examples/native-ink" "$destination/examples/native-ink"
    copy_file "$ROOT/disVM/runtime/test_ink_primitives.c" "$destination/examples/native-ink/stamp-check.c"
    copy_file "$ROOT/ide/dis/tests/native_ink_test.b" "$destination/examples/native-ink/worker-check.b"
    copy_file "$ROOT/compiler/limbo/frontend/NATIVE_C.md" "$destination/docs/limbo/NATIVE_C.md"
    copy_file "$ROOT/compiler/limbo/frontend/NATIVE_C.ru.md" "$destination/docs/limbo/NATIVE_C.ru.md"
    copy_file "$ROOT/flashVM/doc/RENDER_ASYNC.md" "$destination/docs/ide/RENDER_ASYNC.md"
    copy_file "$ROOT/flashVM/doc/RENDER_ASYNC.ru.md" "$destination/docs/ide/RENDER_ASYNC.ru.md"
    copy_file "$ROOT/flashVM/doc/TEXT_COLOR_RANGES.md" "$destination/docs/ide/TEXT_COLOR_RANGES.md"
    copy_file "$ROOT/flashVM/doc/TEXT_COLOR_RANGES.ru.md" "$destination/docs/ide/TEXT_COLOR_RANGES.ru.md"
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
    compiler_enabled=OFF
    if [[ -f "$base/CMakeCache.txt" ]] && grep -q '^DVM_RUNTIME_COMPILER:BOOL=ON$' "$base/CMakeCache.txt"; then
        compiler_enabled=ON
    fi
    printf 'Fircompiler=%s\n' "$compiler_enabled" >>"$destination/NATIVE-MODULES.txt"
    copy_file "$ROOT/disVM/module/fircompiler.m" "$destination/include/limbo/fircompiler.m"
    copy_file "$ROOT/disVM/compiler/compiler_engine.h" "$destination/include/native/compiler_engine.h"
    copy_tree "$ROOT/ide/dis/examples/artifact-export" "$destination/examples/artifact-export"
    copy_file "$ROOT/ide/dis/tests/artifact_test.b" "$destination/examples/artifact-export/acceptance.b"
    copy_file "$ROOT/disVM/runtime/test_artifact_export.py" "$destination/examples/artifact-export/check.py"
    copy_file "$ROOT/disVM/runtime/test_compiler_service.py" "$destination/examples/artifact-export/test_compiler_service.py"
    copy_file "$ROOT/ide/ARTIFACT-EXPORT.md" "$destination/ARTIFACT-EXPORT.md"
    copy_file "$ROOT/ide/ARTIFACT-EXPORT.ru.md" "$destination/ARTIFACT-EXPORT.ru.md"
    copy_tree "$ROOT/ide/dis/examples/mdi" "$destination/examples/mdi"
    copy_tree "$ROOT/ide/dis/examples/debugger-scaffold" "$destination/examples/debugger-scaffold"
    copy_file "$ROOT/disVM/examples/asinctasks/main.b" "$destination/examples/asinctasks/main.b"
    copy_file "$ROOT/disVM/module/asinctasks.m" "$destination/include/limbo/asinctasks.m"
    copy_file "$ROOT/disVM/limbo/asinctasks.b" "$destination/source/asinctasks.b"
    copy_file "$ROOT/ide/dis/tests/asinctasks_test.b" "$destination/examples/asinctasks/acceptance.b"
    copy_file "$ROOT/compiler/runtime/ASINC-TASKS.md" "$destination/examples/asinctasks/README.md"
    copy_file "$ROOT/compiler/runtime/ASINC-TASKS.ru.md" "$destination/examples/asinctasks/README.ru.md"
    copy_file "$ROOT/disVM/examples/asincviews/main.b" "$destination/examples/asincviews/main.b"
    copy_file "$ROOT/disVM/module/asincviews.m" "$destination/include/limbo/asincviews.m"
    copy_file "$ROOT/disVM/limbo/asincviews.b" "$destination/source/asincviews.b"
    copy_file "$ROOT/ide/dis/tests/asincviews_test.b" "$destination/examples/asincviews/acceptance.b"
    copy_file "$ROOT/compiler/runtime/ASINC-VIEWS.md" "$destination/examples/asincviews/README.md"
    copy_file "$ROOT/compiler/runtime/ASINC-VIEWS.ru.md" "$destination/examples/asincviews/README.ru.md"
    copy_file "$ROOT/ide/dis/tests/idemdi_test.b" "$destination/examples/mdi/acceptance.b"
    copy_file "$ROOT/ide/dis/tests/idedebug_test.b" "$destination/examples/debugger-scaffold/acceptance.b"
    copy_file "$ROOT/ide/MDI-SDI.md" "$destination/MDI-SDI.md"
    copy_file "$ROOT/ide/MDI-SDI.ru.md" "$destination/MDI-SDI.ru.md"
    copy_file "$ROOT/ide/DEBUGGER.md" "$destination/DEBUGGER.md"
    copy_file "$ROOT/ide/DEBUGGER.ru.md" "$destination/DEBUGGER.ru.md"
    copy_file "$ROOT/ide/DESKTOP.md" "$destination/DESKTOP.md"
    copy_file "$ROOT/ide/DESKTOP.ru.md" "$destination/DESKTOP.ru.md"
    copy_file "$ROOT/ide/DESKTOP-LIFECYCLE.md" "$destination/DESKTOP-LIFECYCLE.md"
    copy_file "$ROOT/ide/DESKTOP-LIFECYCLE.ru.md" "$destination/DESKTOP-LIFECYCLE.ru.md"
    copy_file "$ROOT/ide/DESKTOP-POINTER.md" "$destination/DESKTOP-POINTER.md"
    copy_file "$ROOT/ide/DESKTOP-POINTER.ru.md" "$destination/DESKTOP-POINTER.ru.md"
    copy_tree "$ROOT/ide/dis/examples/desktop-pointer" "$destination/examples/desktop-pointer"
    copy_tree "$ROOT/ide/dis/examples/desktop-lifecycle" "$destination/examples/desktop-lifecycle"
    copy_file "$ROOT/ide/dis/bridge/idedesktop.b" "$destination/source/idedesktop.b"
    copy_file "$ROOT/ide/dis/bridge/ideatlas.b" "$destination/source/ideatlas.b"
    copy_file "$ROOT/ide/dis/bridge/idedesktopatlas.b" "$destination/source/idedesktopatlas.b"
    copy_file "$ROOT/ide/dis/bridge/idedesktoptools.b" "$destination/source/idedesktoptools.b"
    copy_file "$ROOT/ide/dis/bridge/idedeskpane.b" "$destination/source/idedeskpane.b"
    copy_file "$ROOT/ide/dis/bridge/idedeskpane-ink.b" "$destination/source/idedeskpane-ink.b"
    copy_file "$ROOT/ide/dis/tests/desktop_test.b" "$destination/examples/desktop-owners.b"
    copy_file "$ROOT/ide/dis/bridge/ideartifact.b" "$destination/source/ideartifact.b"
    copy_file "$ROOT/ide/dis/bridge/idefileops.b" "$destination/source/idefileops.b"
    copy_tree "$ROOT/ide/dis/examples/build-panel" "$destination/examples/build-panel"
    copy_tree "$ROOT/ide/dis/examples/compiler-service" "$destination/examples/compiler-service"
    copy_file "$ROOT/ide/dis/tests/compiler_service_test.b" "$destination/examples/compiler-service/acceptance.b"
    copy_file "$ROOT/disVM/compiler/test_engine.c" "$destination/examples/compiler-service/client.c"
    copy_file "$ROOT/disVM/runtime/test_compiler_service.py" "$destination/examples/compiler-service/check.py"
    copy_file "$ROOT/ide/COMPILER-SERVICE.md" "$destination/COMPILER-SERVICE.md"
    copy_file "$ROOT/ide/COMPILER-SERVICE.ru.md" "$destination/COMPILER-SERVICE.ru.md"
    copy_file "$ROOT/ide/dis/bridge/idebuildpanel.b" "$destination/source/idebuildpanel.b"
    copy_file "$ROOT/ide/dis/tests/buildpanel_test.b" "$destination/examples/compiler-service/buildpanel-check.b"
    copy_file "$ROOT/ide/BUILD-PANEL.md" "$destination/BUILD-PANEL.md"
    copy_file "$ROOT/ide/BUILD-PANEL.ru.md" "$destination/BUILD-PANEL.ru.md"
    copy_file "$ROOT/ide/dis/bridge/idecompiler.b" "$destination/source/idecompiler.b"
    copy_file "$ROOT/ide/dis/bridge/ideworkspace.b" "$destination/source/ideworkspace.b"
    copy_tree "$ROOT/ide/dis/examples/workspace" "$destination/examples/workspace"
    sed -e 's|dis/bridge/ideworkspace.m|include/limbo/ideworkspace.m|g' \
        -e 's|dis/bridge/ideworkspace.b|source/ideworkspace.b|g' \
        -e 's|dis/examples/workspace/README.md|examples/workspace/README.md|g' \
        -e 's|../disVM/MODULE-STORE.md|https://github.com/bhgv/flashVM/blob/master/disVM/MODULE-STORE.md|g' \
        "$ROOT/ide/WORKSPACE.md" >"$destination/WORKSPACE.md"
    for language in md ru.md; do
        sed -e 's|dis/bridge/idecompiler.m|include/limbo/idecompiler.m|g' \
            -e 's|../disVM/module/fircompiler.m|include/limbo/fircompiler.m|g' \
            -e 's|../disVM/compiler/compiler_engine.h|include/native/compiler_engine.h|g' \
            -e 's|dis/examples/compiler-service/|examples/compiler-service/|g' \
            -e 's|../disVM/SYNC|https://github.com/bhgv/flashVM/blob/master/disVM/SYNC|g' \
            "$ROOT/ide/COMPILER-SERVICE.$language" >"$destination/COMPILER-SERVICE.$language"
        sed 's|../../../COMPILER-SERVICE|../../COMPILER-SERVICE|g' \
            "$ROOT/ide/dis/examples/compiler-service/README.$language" >"$destination/examples/compiler-service/README.$language"
    done
    if [[ "$compiler_enabled" != ON ]]; then
        rm -f -- "$destination/compiler-service.dis" "$destination/compiler-service.sbl" "$destination/compiler-service-check.dis" "$destination/compiler-service-check.sbl" "$destination/lib/compiler/libdisvm_compiler_engine.a" "$destination/buildpanel-test.dis" "$destination/buildpanel-test.sbl" "$destination/build-panel.dis" "$destination/build-panel.sbl" "$destination/artifact-export.dis" "$destination/artifact-export.sbl" "$destination/artifact-test.dis" "$destination/artifact-test.sbl"
    fi
    for module in ideatlas idevectorlabels idebulk tasks-demo asinctasks capture-demo asincviews ideinkworker idemdi iderails idetools idetoolpalette iderecent idepick idethumbnails idetabview idedocking idesourcedomain idedebug vector-demo idevector idetokenquery tokenquery-demo idesyntax limbolex aslex syntax-check idedeskclip idedeskpane idedesktop desktop-lifecycle desktop-pointer desktop-clipboard; do
        [[ -s "$base/$module.dis" ]] || fail "Missing desktop module: $base/$module.dis; build all runtime targets before installation"
    done
    copy_file "$ROOT/disVM/runtime/run-desktop-lifecycle.sh" "$destination/run-desktop-lifecycle.sh" 755
    copy_file "$ROOT/disVM/runtime/run-desktop-pointer.sh" "$destination/run-desktop-pointer.sh" 755
    copy_file "$ROOT/disVM/runtime/run-desktop-clipboard.sh" "$destination/run-desktop-clipboard.sh" 755
    copy_file "$ROOT/disVM/runtime/run-syntax-check.sh" "$destination/run-syntax-check.sh" 755
    copy_file "$ROOT/disVM/runtime/run-token-query.sh" "$destination/run-token-query.sh" 755
    copy_tree "$ROOT/ide/dis/examples/token-query" "$destination/examples/token-query"
    copy_tree "$ROOT/ide/dis/examples/vector" "$destination/examples/vector"
    for control in ideinkworker ideinkacquire ideinkfit idemdi iderails idetools idetoolpalette iderecent idepick idethumbnails idetabview idedocking idesourcedomain; do
        copy_file "$ROOT/ide/dis/bridge/$control.b" "$destination/source/$control.b"
    done
    copy_file "$ROOT/ide/dis/bridge/idedebug.b" "$destination/source/idedebug.b"
    copy_file "$ROOT/ide/dis/bridge/idevector.b" "$destination/source/idevector.b"
    copy_file "$ROOT/ide/dis/bridge/idevectorlabels.b" "$destination/source/idevectorlabels.b"
    copy_file "$ROOT/ide/dis/bridge/idebulk.b" "$destination/source/idebulk.b"
    copy_file "$ROOT/ide/VECTOR-PICKING.md" "$destination/VECTOR-PICKING.md"
    copy_file "$ROOT/ide/VECTOR-PICKING.ru.md" "$destination/VECTOR-PICKING.ru.md"
    copy_file "$ROOT/ide/dis/bridge/idevectorpresentation.b" "$destination/source/idevectorpresentation.b"
    copy_file "$ROOT/ide/dis/examples/vector/pick-benchmark.b" "$destination/examples/vector/pick-benchmark.b"
    copy_file "$ROOT/ide/dis/bridge/idevectorwire.b" "$destination/source/idevectorwire.b"
    copy_file "$ROOT/ide/VECTOR-EDITING.md" "$destination/VECTOR-EDITING.md"
    copy_file "$ROOT/ide/VECTOR-EDITING.ru.md" "$destination/VECTOR-EDITING.ru.md"
    copy_file "$ROOT/ide/FLASHVM-LOCAL-MOTION.md" "$destination/FLASHVM-LOCAL-MOTION.md"
    copy_file "$ROOT/ide/NODE-TOOLS.md" "$destination/NODE-TOOLS.md"
    copy_file "$ROOT/ide/NODE-TOOLS.ru.md" "$destination/NODE-TOOLS.ru.md"
    copy_file "$ROOT/ide/FLASHVM-LOCAL-MOTION.ru.md" "$destination/FLASHVM-LOCAL-MOTION.ru.md"
    copy_file "$ROOT/ide/VECTOR-CANCELLATION.md" "$destination/VECTOR-CANCELLATION.md"
    copy_file "$ROOT/ide/VECTOR-CANCELLATION.ru.md" "$destination/VECTOR-CANCELLATION.ru.md"
    copy_file "$ROOT/ide/VECTOR-VIEWPORT.md" "$destination/VECTOR-VIEWPORT.md"
    copy_file "$ROOT/ide/VECTOR-VIEWPORT.ru.md" "$destination/VECTOR-VIEWPORT.ru.md"
    copy_file "$ROOT/disVM/MILLISEC.md" "$destination/MILLISEC.md"
    copy_file "$ROOT/disVM/MILLISEC.ru.md" "$destination/MILLISEC.ru.md"
    copy_file "$ROOT/disVM/include/disvm/clock_port.h" "$destination/include/disvm/clock_port.h"
    copy_file "$ROOT/disVM/host/clock_port.c" "$destination/source/clock/clock_port.c"
    copy_file "$ROOT/disVM/runtime/clock_sys.c" "$destination/source/clock/clock_sys.c"
    copy_file "$ROOT/ide/dis/tests/millisec_test.b" "$destination/examples/millisec/main.b"
    copy_file "$base/millisec-test.dis" "$destination/examples/millisec/main.dis"
    copy_file "$ROOT/ide/dis/bridge/idevectorlocal.b" "$destination/source/idevectorlocal.b"
    copy_file "$ROOT/ide/dis/bridge/idevectornodes.b" "$destination/source/idevectornodes.b"
    copy_file "$ROOT/ide/dis/bridge/idevectornodecurve.b" "$destination/source/idevectornodecurve.b"
    copy_file "$ROOT/ide/dis/bridge/idevectornodebreak.b" "$destination/source/idevectornodebreak.b"
    copy_file "$ROOT/ide/dis/bridge/idevectoroverlaycache.b" "$destination/source/idevectoroverlaycache.b"
    copy_file "$ROOT/ide/dis/bridge/idevectorclipboard.b" "$destination/source/idevectorclipboard.b"
    copy_file "$ROOT/ide/dis/bridge/idevectorcancel.b" "$destination/source/idevectorcancel.b"
    copy_file "$ROOT/ide/dis/bridge/idevectorpick.b" "$destination/source/idevectorpick.b"
    copy_file "$ROOT/ide/dis/bridge/idevectorbrushfit.b" "$destination/source/idevectorbrushfit.b"
    for geometry in idevectorbrushgeometry idevectormergedrawing idevectorpainttopology \
        idevectorcutcurve idevectorcutintersections idevectorcutcontacts idevectorcutcoincidence \
        idevectorcutsupport idevectorcutwinding idevectorcutspans idevectorcutboundary \
        idevectorcutgraph idevectorcutoutline idevectorbrushsweep idevectorbrushunion; do
        copy_file "$ROOT/ide/dis/bridge/$geometry.b" "$destination/source/$geometry.b"
    done
    copy_file "$ROOT/ide/dis/bridge/idevectorviewport.b" "$destination/source/idevectorviewport.b"
    copy_file "$ROOT/ide/dis/bridge/idevectoredit.b" "$destination/source/idevectoredit.b"
    copy_file "$ROOT/ide/dis/bridge/idevectorchain.b" "$destination/source/idevectorchain.b"
    copy_file "$ROOT/ide/assets/vector-v0.1718.png" "$destination/VECTOR-EDITOR.png"
    copy_file "$ROOT/ide/VECTOR-EDITOR.md" "$destination/VECTOR-EDITOR.md"
    copy_file "$ROOT/ide/VECTOR-EDITOR.ru.md" "$destination/VECTOR-EDITOR.ru.md"
    copy_file "$ROOT/ide/dis/bridge/idetokenquery.b" "$destination/source/idetokenquery.b"
    copy_file "$ROOT/ide/TOKEN-QUERIES.md" "$destination/TOKEN-QUERIES.md"
    copy_file "$ROOT/ide/TOKEN-QUERIES.ru.md" "$destination/TOKEN-QUERIES.ru.md"
    copy_file "$ROOT/ide/dis/bridge/idesyntax.b" "$destination/source/idesyntax.b"
    copy_tree "$ROOT/ide/syntax" "$destination/source/syntax"
    for file in "$ROOT/ide/syntax/"*.m; do
        copy_file "$file" "$destination/include/limbo/$(basename "$file")"
    done
    copy_file "$ROOT/ide/SYNTAX-LAZY.md" "$destination/SYNTAX-LAZY.md"
    copy_file "$ROOT/ide/SYNTAX-LAZY.ru.md" "$destination/SYNTAX-LAZY.ru.md"

    copy_file "$ROOT/ide/dis/bridge/idedeskclip.b" "$destination/source/idedeskclip.b"
    copy_file "$ROOT/ide/CHROME-ATLAS.md" "$destination/CHROME-ATLAS.md"
    copy_file "$ROOT/ide/CHROME-ATLAS.ru.md" "$destination/CHROME-ATLAS.ru.md"
    copy_file "$ROOT/ide/DESKTOP-WORKSPACE.md" "$destination/DESKTOP-WORKSPACE.md"
    copy_file "$ROOT/ide/DESKTOP-WORKSPACE.ru.md" "$destination/DESKTOP-WORKSPACE.ru.md"
    copy_file "$ROOT/ide/DESKTOP-GLASS-MOTION.md" "$destination/DESKTOP-GLASS-MOTION.md"
    copy_file "$ROOT/ide/DESKTOP-GLASS-MOTION.ru.md" "$destination/DESKTOP-GLASS-MOTION.ru.md"
    copy_file "$ROOT/ide/DESKTOP-CLIPBOARD.md" "$destination/DESKTOP-CLIPBOARD.md"
    copy_file "$ROOT/ide/DESKTOP-CLIPBOARD.ru.md" "$destination/DESKTOP-CLIPBOARD.ru.md"
    copy_tree "$ROOT/ide/dis/examples/desktop-clipboard" "$destination/examples/desktop-clipboard"
    copy_file "$ROOT/ide/ENTRYPOINTS.md" "$destination/ENTRYPOINTS.md"
    copy_file "$ROOT/ide/DESKTOP-PAIRED.md" "$destination/DESKTOP-PAIRED.md"
    copy_file "$ROOT/ide/DESKTOP-PAIRED.ru.md" "$destination/DESKTOP-PAIRED.ru.md"
    copy_file "$ROOT/ide/dis/app/desktop.b" "$destination/source/desktop.b"
    copy_file "$ROOT/ide/dis/app/desktop-ink.b" "$destination/source/desktop-ink.b"
    copy_file "$ROOT/ide/dis/app/desktop.m" "$destination/include/limbo/desktop.m"
    if [[ -f "$base/desktop-paired.dis" ]]; then
        copy_file "$ROOT/ide/dis/app/desktop-paired.b" "$destination/source/desktop-paired.b"
    fi
    install_program "$base" disvm "$destination"
    if [[ "$component" == disvm-editor ]]; then
        for module in desktop-ide desktop-example ide graphical-editor; do
            [[ -s "$base/$module.dis" ]] || fail "Missing desktop application: $base/$module.dis; build all editor targets before installation"
        done
        install_program "$base" disvm-editor "$destination"
        copy_file "$ROOT/disVM/runtime/run-desktop-ide.sh" "$destination/run-desktop-ide.sh" 755
        copy_tree "$ROOT/ide/dis/examples/desktop" "$destination/examples/desktop"
        copy_file "$ROOT/disVM/runtime/test_desktop.py" "$destination/examples/desktop/check.py"
        copy_file "$ROOT/disVM/runtime/test_desktop.py" "$destination/examples/desktop/test_desktop.py"
        copy_file "$ROOT/disVM/runtime/test_desktop_pointer.py" "$destination/examples/desktop/check-pointer.py"
        copy_file "$ROOT/disVM/runtime/test_desktop_clipboard.py" "$destination/examples/desktop/check-clipboard.py"
        copy_file "$ROOT/disVM/runtime/test_desktop_workspace.py" "$destination/examples/desktop/check-workspace.py"
        copy_file "$ROOT/disVM/runtime/test_graphical_editor.py" "$destination/examples/desktop/test_graphical_editor.py"
        copy_file "$ROOT/ide/assets/desktop-v0.1719.png" "$destination/DESKTOP.png"
        copy_file "$ROOT/ide/assets/desktop-v0.1719.png" "$destination/assets/desktop-v0.1719.png"
        copy_file "$ROOT/ide/assets/desktop-v0.1677.png" "$destination/DESKTOP-CLIPBOARD.png"
        copy_tree "$ROOT/ide/as" "$destination/source/as"
        copy_file "$ROOT/compiler/as2/view/README.md" "$destination/docs/as2/VIEW.md"
        copy_file "$ROOT/compiler/as2/view/README.ru.md" "$destination/docs/as2/VIEW.ru.md"
        copy_file "$ROOT/fir/v1/doc/AS_PROPERTIES.md" "$destination/docs/fir/AS_PROPERTIES.md"
        copy_file "$ROOT/fir/v1/doc/AS_PROPERTIES.ru.md" "$destination/docs/fir/AS_PROPERTIES.ru.md"
        for artifact in fir opt.fir swo swf; do
            if [[ -f "$base/native/desktop-view.$artifact" ]]; then
                copy_file "$base/native/desktop-view.$artifact" "$destination/ui/desktop-view.$artifact"
            fi
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
    fi
    install_runtime_payload
}
