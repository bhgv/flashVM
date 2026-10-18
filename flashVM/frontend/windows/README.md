# Windows desktop launcher / Запуск в Windows

Source / Исходник: [main.c](main.c). Win32 `GetCommandLineW` and
`CommandLineToArgvW` retain Unicode filenames; `WideCharToMultiByte` converts each
argument to UTF-8 before shared parsing. Partial allocations and the native argv
array are released on every path. SDL's file adapter opens those UTF-8 paths.
This is a console executable with a real resizable SDL window, not a file association.

Install a Windows C/C++ toolchain, CMake and an SDL2 development package exposing
`SDL2::SDL2` (for example an SDL2 package in vcpkg). Configure from flashVM:

```powershell
cmake -S . -B build-desktop -DFVM_BUILD_DESKTOP_FRONTEND=ON -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build-desktop --config Release --target flashvm_player
.\build-desktop\frontend\Release\flashvm_player.exe --help
.\build-desktop\frontend\Release\flashvm_player.exe --jit --speed 150% "C:\Movies\movie.swf"
```

Use `-DSDL2_DIR=...` or `-DCMAKE_PREFIX_PATH=...` with another SDK. Multi-config
paths include Release/Debug; single-config generators place the executable directly
in `frontend`. Supply SDL2.dll and any selected SDK runtime dependencies next to
the executable or through the runtime search path, preserving their license notices.
The VM's bundled dependencies remain static. A Windows native build/run has not
been executed here; Linux testing is not Windows acceptance.

Нужны Windows toolchain, CMake и SDL2 SDK. При Visual Studio путь содержит Release
или Debug; у Ninja обычно нет этой дополнительной папки. Динамический SDL2 SDK
требует доступного SDL2.dll и его зависимостей. Лицензии SDK сохраняются отдельно.
Нативный Windows-прогон пока не выполнен. [Руководство](../../doc/DESKTOP_FRONTEND.ru.md).
