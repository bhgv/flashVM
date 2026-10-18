# macOS desktop launcher / Запуск в macOS

Source / Исходник: [main.c](main.c). The launcher remains on the process main
thread; SDL2 owns Cocoa event/render integration. No custom Objective-C GUI or
background VM thread is introduced. This is a command-line launcher with an OS
window, not a packaged .app or document association.

Install the Apple command-line C/C++ tools, CMake and an SDL2 development SDK
with its CMake package, then from flashVM:

```sh
cmake -S . -B build-desktop -DFVM_BUILD_DESKTOP_FRONTEND=ON -DCMAKE_PREFIX_PATH=/path/to/SDL2/prefix
cmake --build build-desktop --target flashvm_player --parallel
./build-desktop/frontend/flashvm_player --help
./build-desktop/frontend/flashvm_player --no-jit --quality high --speed 100 movie.swf
```

Use the signing/SDK instructions in the VM's JIT macOS guides before requesting
`--jit`, particularly for opt-in ARM64 MAP_JIT. Backend availability alone does
not certify executable-memory permission. Interpreter mode avoids native JIT
publication. SDL2 runtime libraries must be discoverable with preserved notices.
No native macOS run or app-bundle/signing acceptance is claimed here.

Нужны Apple toolchain и SDL2 SDK. Окно и VM работают в главном потоке. Для JIT
применяются существующие ограничения и подготовка подписи из руководств
[JIT_MACOS_X64](../../doc/JIT_MACOS_X64.md) и
[JIT_MACOS_ARM64](../../doc/JIT_MACOS_ARM64.md). Нативная macOS-проверка и .app-пакет
пока отсутствуют. [Руководство](../../doc/DESKTOP_FRONTEND.ru.md).
