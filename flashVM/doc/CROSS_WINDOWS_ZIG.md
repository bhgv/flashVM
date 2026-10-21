# Windows x64 cross-builds with Zig (v0.770)

[Русский](CROSS_WINDOWS_ZIG.ru.md) · [Linux targets](CROSS_LINUX_ZIG.md) · [platform acceptance](PLATFORM_VALIDATION.md)

The explicit [Windows toolchain](../cmake/toolchains/zig-windows.cmake) builds
C89/C++98 for `x86_64-windows-gnu`. Supply a complete Zig distribution for the
build host, with its standard library, and an absolute executable path. Validation
uses Zig 0.13.0 and CMake 4.4 on x86-64 Linux. Other hosts/compiler versions need
separate checks. This target is Windows/MinGW, separate from Linux/musl and Android.
The older [MinGW toolchain](../cmake/toolchains/mingw-x64.cmake) remains available.

Zig supplies the compiler, archiver, target headers and compiler runtimes; it does
not supply SDL. Keep target-built SDKs separate from build-host libraries. The
profile links project, C++/exception support and SDL libraries statically while
importing Windows system DLLs, including UCRT API sets. The system C runtime is
still an OS dependency. This is not a promise of zero DLL imports or of compatibility with
an arbitrary Windows release.

## Minimal core and console programs

From the component directory, replace `/opt/zig/zig` with the actual installation:

```sh
cmake -G "Unix Makefiles" -S . -B build-win-core \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/zig-windows.cmake \
  -DFVM_ZIG_EXECUTABLE=/opt/zig/zig \
  -DFVM_ZIG_TARGET=x86_64-windows-gnu \
  -DFVM_DEFS_FILE=cmake/profiles/minimal.defs \
  -DFVM_BUILD_TESTS=ON -DFVM_BUILD_EXAMPLES=ON \
  -DFVM_BUILD_REFERENCE_RUNNER=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-win-core --target \
  flashvm_tests_swf_stream_storage flashvm_tests_swf_stream_pcm_reader_api \
  flashvm_tests_swf_stream_adpcm_allocation flashvm_examples_multimedia_swf_stream_main \
  --parallel 3
```

The whole minimal core is compiled and linked into the selected executables.
This does not build every optional dependency/profile or execute the tests.
CMake omits host process/timezone harnesses for cross builds. Do not run cross
CTest on the build host as if it were native acceptance.

Both Zig inputs are mandatory and frozen in a configured build directory. Use
fresh directories for another compiler installation/target. Missing/relative
compiler paths, unsupported targets and directories used as compiler paths are
rejected. The toolchain uses static try_compile archives; selected final program
links remain necessary. Target library/include/package discovery is isolated;
host build programs remain discoverable.

Copy **both** `zig-windows.cmake` and
[`zig-windows-archives.cmake`](../cmake/toolchains/zig-windows-archives.cmake)
when moving the toolchain. The companion reapplies Zig `ar`/`ranlib` after CMake's
Windows compiler platform rules and repairs the GNU Make temporary-object archive
link rule. No shell wrapper, host GCC archiver or hard-coded repository path is
required by the distributed toolchain. It does not configure a Windows RC/resource
compiler or promise arbitrary third-party build-system compatibility.

## Build a real target SDL SDK

Use an upstream SDL **2.30.0** source distribution (retain its license), outside
FlashVM. A build-host SDL installation cannot replace this target SDK. The supplied
[initial cache](../cmake/sdk/sdl2-windows-static.cmake) requests a static archive,
Windows windows, WASAPI/WinMM and dummy drivers, without SDL2main, external resampler,
optional GPU integrations or joystick/haptic/sensor support. WinMM is enabled by
SDL's desktop-Windows audio configuration, not a fictitious SDL_WMM cache option.
The player supplies its own UTF-16 command-line adapter and software presentation.

For SDL sources at `../SDL-2.30.0`:

```sh
cmake -G "Unix Makefiles" -S ../SDL-2.30.0 -B build-win-sdl \
  -C "$PWD/cmake/sdk/sdl2-windows-static.cmake" \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/cmake/toolchains/zig-windows.cmake" \
  -DFVM_ZIG_EXECUTABLE=/opt/zig/zig \
  -DFVM_ZIG_TARGET=x86_64-windows-gnu \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PWD/sdk-win"
cmake --build build-win-sdl --target install --parallel 3
```

## Build the player and embedding library

```sh
cmake -G "Unix Makefiles" -S . -B build-win-player \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/zig-windows.cmake \
  -DFVM_ZIG_EXECUTABLE=/opt/zig/zig \
  -DFVM_ZIG_TARGET=x86_64-windows-gnu \
  -DFVM_DEFS_FILE=cmake/profiles/player-jit.defs \
  -DSDL2_DIR="$PWD/sdk-win/lib/cmake/SDL2" \
  -DFVM_BUILD_DESKTOP_FRONTEND=ON -DFVM_ENABLE_SDL_AUDIO=ON \
  -DFVM_PLAYER_SOCKET_HTTP=ON -DFVM_BUILD_TESTS=OFF \
  -DFVM_BUILD_EXAMPLES=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-win-player --target flashvm_player --parallel 3
```

The selected profile includes AGG, zlib, native Windows threads/OS ports, interpreter/
JIT code and the SDL player host. The socket option links native Winsock HTTP;
HTTPS still requires the separately documented TLS provider and target SDK.
`flashvm_player_host` is also a static embedding archive in this configuration;
use its C headers and transitive CMake dependencies, rather than adding a CLI
entry point to an IDE. Shared-host/DLL packaging is a separate build/acceptance.
Fonts, image codecs, SQLite and general multimedia SDKs are disabled by this
profile; do not infer their Windows build/runtime acceptance from it.

## Inspect linked images without executing them

```sh
python3 scripts/check_windows_pe_inspector.py
python3 scripts/check_zig_windows_toolchain.py \
  --zig /opt/zig/zig --target x86_64-windows-gnu \
  --work /tmp/flashvm-windows-toolchain-check \
  --artifact build-win-core/flashvm_tests_swf_stream_storage.exe \
  --artifact build-win-player/frontend/flashvm_player.exe
```

The work directory must be absent/empty. The checker copies the two-file toolchain
into a detached path with spaces, builds a strict C89 archive and C++98 exception
program, checks PE identity/imports, repeat configure, rejected input changes and
the missing-companion failure. It never launches the target programs. The Python
inspector test is also registered as native `flashvm_windows_pe_inspector` CTest.

[`inspect_windows_pe.py`](../scripts/inspect_windows_pe.py) accepts this executable
subset: bounded DOS/COFF/PE32+ headers, x64 executable flags, mapped non-overlapping
sections, a terminated import table and direct Windows-component/API-set DLL names.
DLLs, PE32, another CPU, delay imports, malformed ranges and unknown direct imports
are rejected. This is a build-profile check, **not** a complete loader, signature,
malware or dependency-security audit. It does not follow transitive imports or
`LoadLibrary` calls, and an allowed API-set spelling does not prove OS availability.
Run the inspector directly for additional selected executables:

```sh
python3 scripts/inspect_windows_pe.py build-win-player/frontend/flashvm_player.exe
```

## Windows runtime acceptance remains separate

Copy the selected output/resources to Windows and test there, for example from
PowerShell in the component directory with the target build available:

```powershell
.\build-win-player\frontend\flashvm_player.exe --version
$env:SDL_VIDEODRIVER = "dummy"
$env:SDL_AUDIODRIVER = "dummy"
.\build-win-player\frontend\flashvm_player.exe --once --stats examples/player/pcm_stream/adpcm-stream.swf
.\build-win-core\flashvm_tests_swf_stream_storage.exe
.\build-win-core\flashvm_tests_swf_stream_pcm_reader_api.exe
```

Then test a real Windows window/audio device, paths, HTTP, pause/navigation, debugger
and interpreter/JIT equivalence with dummy variables removed. Dummy-driver success
alone is not native window/audio acceptance. No Windows/Wine execution is claimed
by a cross-link or by the PE inspection. All nine roadmap stages remain open.

Primary references: [Zig 0.13 release](https://ziglang.org/download/0.13.0/release-notes.html),
[Microsoft PE/COFF format](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format),
[SDL Windows build notes](https://wiki.libsdl.org/SDL2/README-windows).

## Revision evidence and remaining acceptance

The detached compiler fixture, repeat configuration and six rejection checks passed.
The complete minimal core linked all four listed programs. SDL 2.30.0 was built
and installed for the target; generated configuration enables Windows video,
WASAPI, WinMM and dummy audio/video. The full player profile links the Windows
entry adapter, static host/SDL/AGG/zlib and native Winsock HTTP. The fixture plus
five project executables pass PE32+ x64/direct-import checks. Imports include
Windows UCRT API sets, Kernel32 and Winsock; the player also imports native GUI/COM/
WinMM components. No SDL2.dll or separate C++/pthread runtime appears as a direct
import in these checked images. This does not inspect delayed manual DLL loads.

The host inspector test checks one valid synthetic image and 11 independent
malformed/wrong-profile vectors. No Windows/Wine process was run on this host.
Unix Makefiles was the checked generator; other generators and shared-host DLL
packaging need separate checks. The restricted host used an uncommitted Zig
self-location launcher; the delivered toolchain requires only ordinary Zig with
its adjacent library and the documented companion, not that launcher.
