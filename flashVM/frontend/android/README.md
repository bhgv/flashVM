# Android ARM64 test player / Тестовый плеер

v0.582 adds a real SDLActivity/Surface/native APK host, with menu selection for
[two bundled SWFs](../../examples/player/animation_audio/README.md) and
interpreter/JIT. Android 6/API23+, arm64-v8a. This is not a desktop main stub:
[main.c](main.c) exports SDL_main from libflashvm_android.so;
[java/](java/org/bhgv/flashvm/MenuActivity.java) contains the menu and
[SDLActivity subclass](java/org/bhgv/flashvm/PlayerActivity.java).
[Manifest](AndroidManifest.xml) runs the player in :player, leaving the menu in
its own process. Back exits a test, foreground/background coordinates SDL/audio/
timeline lifecycle. Touch maps to primary mouse; hardware arrows/Space work.

Это настоящий APK с Surface, SDL Java/JNI и двумя встроенными SWF, но пока без
SAF/file browser/произвольного ACTION_VIEW intent. Копии SWF находятся в приватной
папке; storage/mic/network permissions не нужны. Экранной клавиатуры и multitouch
recognizer нет. Меню выбирает test/mode/mute; ошибки native app видны в logcat.
Полные поля HostInput и ограничения звука:
[RU](../../doc/PLAYER_TEST_BUILDS.ru.md) / [EN](../../doc/PLAYER_TEST_BUILDS.md).

## SDK preparation / Подготовка

Install/extract a legitimate Android NDK r27c, Android platform35/android.jar,
Build Tools35 (aapt2/d8/zipalign/apksigner), JDK17, CMake, Python3 and SDL2 2.30.0
source SDK. These are explicit external build tools, not vendored FlashVM source.
No Gradle/Maven downloads occur in the packaging script. Tool paths below are
examples: set them to your installed SDKs. FlashVM remains detachable from the
workspace/compiler/linker. Its shared MIT LICENSE must accompany detached copies.

Сенсоры SDL2 2.30.0 отключаются: эта SDK-версия вызывает ALooper_pollAll,
удалённый заголовками NDK27. Плеер здесь использует Surface/input/audio, не SDL
sensor subsystem; VM device APIs не подменяются. Новые SDK можно проверять отдельно.
Все native зависимости должны быть arm64-v8a/API23+; включается flexible 16 KiB
page alignment для современных устройств. Нативный запуск на реальном телефоне
пока не проверен, независимо от успешной сборки и подписи APK.

## Build / Сборка

From the repository root (or adapt -S/profile paths for a detached VM component):

```sh
export FVM_NDK=/path/to/android-ndk-r27c
export FVM_SDL_SOURCE=/path/to/SDL2-2.30.0
export FVM_ANDROID_JAR=/path/to/platforms/android-35/android.jar
export FVM_BUILD_TOOLS=/path/to/build-tools/35.0.0
export FVM_JAVA_HOME=/path/to/jdk-17

cmake -S "$FVM_SDL_SOURCE" -B /tmp/fvm-sdl-android -DCMAKE_TOOLCHAIN_FILE="$FVM_NDK/build/cmake/android.toolchain.cmake" -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-23 -DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON -DSDL_SENSOR=OFF -DSDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_TEST=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/tmp/fvm-android-sdl
cmake --build /tmp/fvm-sdl-android --parallel
cmake --install /tmp/fvm-sdl-android

cmake -S flashVM -B /tmp/fvm-android -DCMAKE_TOOLCHAIN_FILE="$FVM_NDK/build/cmake/android.toolchain.cmake" -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-23 -DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON -DANDROID_STL=c++_static -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs" -DFVM_BUILD_DESKTOP_FRONTEND=ON -DFVM_ENABLE_SDL_AUDIO=ON -DSDL2_DIR=/tmp/fvm-android-sdl/lib/cmake/SDL2 -DFVM_BUILD_TESTS=OFF -DFVM_BUILD_EXAMPLES=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/fvm-android --target flashvm_player --parallel

python3 flashVM/frontend/android/build_apk.py --native /tmp/fvm-android/frontend/libflashvm_android.so --sdl-lib /tmp/fvm-android-sdl/lib/libSDL2.so --sdl-source "$FVM_SDL_SOURCE" --android-jar "$FVM_ANDROID_JAR" --build-tools "$FVM_BUILD_TOOLS" --java-home "$FVM_JAVA_HOME" --work /tmp/fvm-apk --output "$PWD/FlashVM-player-android-arm64-v0.582.apk" --strip "$FVM_NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip"
```

Use the NDK host-tag path for your actual build OS in --strip. `--strip` is optional.
The [packager](build_apk.py) regenerates classes/dex/assets under the explicitly
selected work folder, copies both movies and MIT/AGG/zlib/SDL notices, checks
AArch64 ELF and PT_LOAD alignment, aligns native library ZIP entries and verifies
the signed result. It owns no device/emulator and does not install the app.
The disposable RSA test key stays in work/test.keystore; reuse it for subsequent
local test upgrades, never use it as a production release identity. Keep SDK/
work/build output out of the source repository. Libraries are copied before strip.

## Device verification / Проверка телефона

Скачайте APK на телефон, установите и сначала выберите Interpreter/animation,
затем JIT/animation. Проверьте движение и три тихих тона. Далее оба input-теста:
касание двигает розовый указатель/жёлтый счётчик, BT/USB клавиатура — синий квадрат.
Проверьте Home→возврат, Back→меню, повторный запуск каждого режима и Mute.
При ошибке приложите logcat (фильтр FlashVM/SDL/AndroidRuntime). Например, с ПК:

```sh
adb install -r FlashVM-player-android-arm64-v0.582.apk
adb logcat -v threadtime
```

JIT availability includes the backend/memory policy; a device may reject executable
mappings or use per-body interpreter fallback. `--stats` logs actual counts on
exit, not merely the selected menu label. No phone/Android emulator execution or
physical audio acceptance was performed in the build environment. APK signing,
16 KiB alignment and an ARM64 ELF are build evidence only.
