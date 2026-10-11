# Offline build and platform validation

CMake 3.10+ uses strict C90/C++98, with pedantic errors on GCC/Clang. Bundled
zlib and AGG sources compile with the project. No configure-time downloads.

Linux/macOS: configure `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`, build
`cmake --build build --parallel`, test `ctest --test-dir build --output-on-failure`.
Windows: choose a supported C/C++ compiler/generator, configure the same way,
then `cmake --build build --config Release` and
`ctest --test-dir build -C Release --output-on-failure`. C++98 is not selectable
as a separate MSVC language mode; compatibility still requires a real MSVC run.
Android: pass the NDK's android.toolchain.cmake, ANDROID_ABI and
ANDROID_PLATFORM at configuration; tests must execute on a device/emulator,
not on the build host. NDK and platform SDKs are not bundled.

Options: FVM_BUILD_TESTS, FVM_BUILD_EXAMPLES, FVM_ENABLE_AGG,
FVM_ENABLE_OS_PORTS, FVM_ENABLE_RENDER_JOBS and optional FVM_ENABLE_SQLITE.
Tests/examples with disabled adapters are excluded. Disabling tests and
examples builds libraries only. AGG can be disabled; the raster API then
returns UNSUPPORTED. SQLite remains an optional system dependency. No JIT or
media backend is available; requesting either produces a configuration error.

CMake registers every discovered regression/example entry point through a
single helper, preserves assertions in Release, and sets a 30-second timeout.
Rerun configuration after adding tests. `scripts/check.py` remains the Linux
strict/sanitizer runner; SQLite is excluded, zlib and AGG are compiled from
source. Sanitizer exception/LeakSanitizer limits are recorded in VALIDATION.

Only Linux execution is currently validated. Platform adapter selection and
portable source do not establish Windows/macOS/Android readiness. Older
validation reports remain historical; see `doc/validation/phase2-*` for this
checkpoint. Tool binaries are not included in the source ZIP.
