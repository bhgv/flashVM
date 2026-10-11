# Offline builds and platform validation

CMake 3.10+ selects strict C90/C++98 modes, with pedantic errors on GCC/Clang
and errors for undeclared C functions. All required third-party source is in
`external/`. No configure-time download or system Cairo/SQLite fallback occurs.
The `-S`/`-B` command form requires CMake 3.13+; older versions can configure
from a build directory with `cmake ..`.

Edit top-level DEFS.txt or provide FVM_DEFS_FILE; see
[build definitions](BUILD_DEFINITIONS.md). AUTO_PLATFORM follows the target
toolchain; explicit OS keys must match it. Default: AGG, bundled SQLite, HEADLESS.
Cairo/Pixman are selected with GRAPHICS_CAIRO. NO_GRAPHICS builds the raster API
as unsupported. Disabled SQLite/input/render-job programs are excluded.

Linux/macOS: configure `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`, build
`cmake --build build --parallel`, test `ctest --test-dir build --output-on-failure`.
Windows: choose the compiler/generator, configure, then
`cmake --build build --config Release` and
`ctest --test-dir build -C Release --output-on-failure`. MSVC has no separate
C++98 switch; compatibility requires an actual MSVC build and execution.
Android: pass the NDK android.toolchain.cmake, ANDROID_ABI and ANDROID_PLATFORM;
tests execute on a device/emulator. NDK and SDKs are not included.

The scalar Cairo/Pixman profile keeps platform threading and native 64-bit types,
but avoids display/font packages and CPU assembler flags. SQLite retains its
own POSIX/Windows VFS under external/. FlashVM native adapters remain in ports/.
Compiler toolchains/system headers and OS threading/math libraries are required.

CMake discovers test/example entry points, keeps assertions active in Release,
and sets a 30-second timeout. Rerun configuration after adding a test source.
`scripts/check.py` provides a Linux standalone strict/sanitizer build from the
same DEFS.txt and vendor source lists. Sanitizer limits are recorded in
[checkpoint validation](DEPENDENCY_CHECKPOINT.md).

Only Linux x86_64 execution is validated. Portable source/configuration does
not establish Windows/macOS/Android readiness. FB/GLES/OGL/DX/SDL1/SDL2/SDL3,
JIT and media are reserved keys; selecting them produces a configuration error.

FONTS adds bundled static FreeType 2.14.3, HarfBuzz 1.8.8, libunibreak 6.1 and
FriBidi 1.0.16, plus AGG/Cairo FreeType adapters. Fontconfig/system font discovery
remains disabled. [Profile and compatibility limits](FONT_LIBRARIES.md).
