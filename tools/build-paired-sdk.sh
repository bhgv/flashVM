# Copyright (c) 2026 bhgv
# EN: Build the SDK and general-purpose FlashVM player used by the paired workspace package.
# RU: Собрать SDK и штатный плеер FlashVM для установленной парной IDE.


# EN: Borrow workspace build paths, configuration and SDL2 SDK from the caller.
# Stage packages under the build root. Abort on configure, build or install failure.
# RU: Использовать пути сборки, конфигурацию и SDL2 SDK вызывающего. Установить
# SDK внутри build root. Ошибки конфигурации, сборки и установки прерывают работу.
build_paired_sdk() {
    local prefix="$build_root/paired-sdk" build component source
    local options=()
    for component in styx libunibus styxrpc; do
        build="$build_root/paired-$component"
        source="$ROOT/lib/$component"
        options=()
        if [[ "$component" == styxrpc ]]; then
            options+=(-DSTYXRPC_HOSTED=ON)
        fi
        "$cmake" -S "$source" -B "$build" "-DCMAKE_BUILD_TYPE=$config" -DBUILD_TESTING=OFF \
            "-DCMAKE_PREFIX_PATH=$prefix" "-DCMAKE_INSTALL_PREFIX=$prefix" "${options[@]}"
        "$cmake" --build "$build" --config "$config" --target install -- "-j$jobs"
    done
    "$cmake" -S "$ROOT/disVM/native" -B "$build_root/paired-native" "-DCMAKE_BUILD_TYPE=$config" \
        "-DINFERNO_HOST_SUPPORT=$ROOT/tools/limbo-bootstrap" "-DINFERNO_VFS_SUPPORT=$ROOT"
    "$cmake" --build "$build_root/paired-native" --config "$config" --target terminal-ordered-files -- "-j$jobs"
    "$cmake" -S "$ROOT/flashVM" -B "$build_root/paired-flashvm" "-DCMAKE_BUILD_TYPE=$config" \
        "-DFVM_DEFS_FILE=$ROOT/flashVM/cmake/profiles/text-jit.defs" "-DCMAKE_PREFIX_PATH=$prefix" \
        "-DSDL2_DIR=$SDL2_SDK_DIR" -DFVM_ENABLE_STYXRPC=ON -DFVM_BUILD_DESKTOP_FRONTEND=ON \
        -DFVM_BUILD_TESTS=OFF -DFVM_BUILD_EXAMPLES=OFF
    "$cmake" --build "$build_root/paired-flashvm" --config "$config" --target flashvm_styx_player -- "-j$jobs"
}
