# Linux/musl cross-builds with Zig

[Русский](CROSS_LINUX_ZIG.ru.md) · [platform acceptance](PLATFORM_VALIDATION.md)

The standalone [toolchain](../cmake/toolchains/zig-linux.cmake) selects one explicit
Linux target: `aarch64-linux-musl`, `riscv64-linux-musl` or `x86_64-linux-musl`.
It uses `zig cc`, `zig c++`, `zig ar` and `zig ranlib`; no host GCC archive tool or
shell wrapper is required. Public headers remain C89, implementation C++98.
The default executable link is static; the minimal profile needs no SDL SDK.
This is a Linux/musl build, **not** an Android/Bionic, Windows or macOS build.
Those platforms need their own NDK/toolchain/SDK; see the existing platform guide.

Install a complete Zig distribution for the **build host**, CMake, a supported
build generator (Make or Ninja) and Python 3 for the optional fixture check.
Use the absolute path to the installed Zig executable, retaining its adjacent
standard library. Validation for this revision uses Zig 0.13.0 and CMake 4.4;
other compiler releases need their own checks. A first build compiles Zig's target
C/C++ runtimes and can take longer than subsequent builds. Keep caches writable.
Commands below start **inside the detached `flashVM/` component** and do not
require sibling compiler/linker repositories. Carry the common MIT license when
redistributing a detached component.

## Build the complete minimal core and selected programs

Replace `/opt/zig/zig` with the actual absolute executable. Use a separate build
directory for every target, compiler installation and feature profile.

```sh
cmake -S . -B build-arm64-musl \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/zig-linux.cmake \
  -DFVM_ZIG_EXECUTABLE=/opt/zig/zig \
  -DFVM_ZIG_TARGET=aarch64-linux-musl \
  -DFVM_DEFS_FILE="$PWD/cmake/profiles/minimal.defs" \
  -DFVM_BUILD_TESTS=ON -DFVM_BUILD_EXAMPLES=ON \
  -DFVM_BUILD_REFERENCE_RUNNER=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-arm64-musl --parallel 4 --target \
  flashvm_reference_swf \
  flashvm_tests_swf_stream_output_segments \
  flashvm_tests_movieclip_playback_info \
  flashvm_examples_multimedia_swf_stream_main
```

The programs link the actual `libflashvm.a` and bundled static zlib; they are not
single-file compile probes. `minimal.defs` disables graphics, fonts, image codecs,
SQLite and media adapters while retaining the VM, SWF loader and native OS ports.
The stream tutorial's prepared PCM conversion does not need a physical device.
Use `riscv64-linux-musl` with a new `build-riscv64-musl` directory for RISC-V64.
For x86-64/musl select `x86_64-linux-musl` and another directory.

Missing/relative compiler paths, directories in place of executables and unsupported
or missing targets stop configuration. Changing either explicit Zig input in an
existing cache also stops configuration; create a fresh build directory. Manually
overriding CMake compiler/target flags defeats this toolchain's intended contract.
Do not mix host and target headers/libraries. Executable discovery uses the host;
include/library/package searches use the configured target roots. For additional
SDKs supply explicit target-built package locations and `CMAKE_FIND_ROOT_PATH`.

## Check the toolchain independently

```sh
python3 scripts/check_zig_linux_toolchain.py \
  --zig /opt/zig/zig --target aarch64-linux-musl \
  --work /tmp/flashvm-arm64-toolchain-check
```

The output directory must be absent or empty. `--cmake` selects a CMake executable.
The check copies only the toolchain into a detached path containing spaces, builds
a strict C89 static library and C++98 exception-using executable, checks the archive
signature and ELF CPU/type/program headers, then checks repeated configuration,
rejected target/compiler changes and invalid inputs. It **does not execute** that executable.
Logs/generated sources stay in the selected directory, outside tracked source.
A correct ELF header is build evidence, not proof of working target instructions,
OS services, audio, networking or devices.

## Build and link the SDL player for ARM64

Use a separate, unchanged SDL 2.30.0 source distribution at `/opt/src/SDL2`.
The [initial SDK cache](../cmake/sdk/sdl2-linux-dummy.cmake) selects a static,
dummy-audio/dummy-video SDK without host graphics/audio dependencies. This is
for build/link validation and software/dummy execution, not physical devices.
The paths below are examples: choose writable absolute build/install locations.

```sh
cmake -S /opt/src/SDL2 -B /tmp/sdl-arm64-build \
  -C "$PWD/cmake/sdk/sdl2-linux-dummy.cmake" \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/cmake/toolchains/zig-linux.cmake" \
  -DFVM_ZIG_EXECUTABLE=/opt/zig/zig -DFVM_ZIG_TARGET=aarch64-linux-musl \
  -DCMAKE_INSTALL_PREFIX=/tmp/sdl-arm64-sdk -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/sdl-arm64-build --target install --parallel 4
cmake -S . -B build-player-arm64 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/zig-linux.cmake \
  -DFVM_ZIG_EXECUTABLE=/opt/zig/zig -DFVM_ZIG_TARGET=aarch64-linux-musl \
  -DFVM_DEFS_FILE="$PWD/cmake/profiles/player-jit.defs" \
  -DSDL2_DIR=/tmp/sdl-arm64-sdk/lib/cmake/SDL2 \
  -DFVM_BUILD_DESKTOP_FRONTEND=ON -DFVM_ENABLE_SDL_AUDIO=ON \
  -DFVM_BUILD_TESTS=OFF -DFVM_BUILD_EXAMPLES=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-player-arm64 --target flashvm_player --parallel 4
```

The player binary is `build-player-arm64/frontend/flashvm_player`.
This profile uses bundled static AGG rasterization, omits fonts/image codecs/SQLite
and enables JIT selection. Optional HTTP/TLS providers remain off. Actual ARM64 JIT
execution must be checked separately; compiling a backend does not execute it.
With this SDK a real target can use `SDL_VIDEODRIVER=dummy` and
`SDL_AUDIODRIVER=dummy` with the [player's finite-run options](../frontend/README.md).
For physical X11/Wayland/audio output build a separate target SDK with the actual
system's headers, libraries and enabled drivers; do not reuse this dummy SDK as
evidence of native window/device support. Keep SDL's upstream notices with SDKs.

## Execute on the selected target

Copy executable files and inputs to a compatible Linux target, preserve/set their
executable permissions, and run there. For example:

```sh
./flashvm_tests_swf_stream_output_segments
./flashvm_tests_movieclip_playback_info
./flashvm_examples_multimedia_swf_stream_main converted.pcm --convert
```

Use the tutorial's [documented arguments and PCM checks](../examples/multimedia/swf_stream/README.md).
The reference runner needs an explicit SWF input; see [reference checks](SWF_REFERENCE_RUNNER.md).
Never run a foreign binary on the build host as a substitute for target acceptance.
An explicitly installed emulator can be configured with
`-DCMAKE_CROSSCOMPILING_EMULATOR=/absolute/path/to/qemu-aarch64` in a fresh build.
CTest's executable tests then use that adapter. Host Python process harnesses are
omitted by FlashVM cross configuration; emulator presence does not make them run.
Without an execution adapter, treat the build as **build-only** and do not run
CTest expecting it to launch foreign binaries. `validate_platform.py` intentionally
rejects cross directories; it is the native platform acceptance runner.

## Evidence and remaining work

v0.762 validation: complete Release minimal core plus the four listed executables
built and linked for all three triples. ELF checks confirmed all twelve executable
CPU identifiers, load segments and absence of `PT_INTERP`. The ARM64 SDL player
also linked statically with target-built SDL 2.30.0 and bundled AGG. No ARM64 or
RISC-V64 program was executed: QEMU/target machines were unavailable.
The detached toolchain fixture passed for all three triples, including compiler/
target-change rejection, paths with spaces and five invalid-input cases.
On this x86-64 Linux host the x86-64/musl binaries actually passed the stream
segment and MovieClip playback-info fixtures, the stream tutorial's exact PCM/
error cases (including the independent FFmpeg oracle), and generated SWF reference
runner cases. This is alternate-libc native execution, not foreign-CPU execution.
This restricted build host used an uncommitted validation-only launcher for Zig
executable self-location; the distributed toolchain calls ordinary Zig directly
and does not install or require that launcher.
Generated logs, installed SDKs and tool caches are outside the source repository.

See [current stage status](ROADMAP_STATUS.md). Linux native device/window checks,
reference Flash compatibility, all other operating systems and execution on ARM64
and RISC-V64 remain separate acceptance gates. A static musl binary is not a claim
that every Linux kernel, CPU extension set or system service is supported.

`--artifact PATH` can be repeated to check already-linked FlashVM executables
against the same target ELF rules after the detached fixture succeeds.

Windows x64 explicit Zig/static-SDL build instructions (v0.770): [EN](CROSS_WINDOWS_ZIG.md) / [RU](CROSS_WINDOWS_ZIG.ru.md). Build/link evidence remains separate from Windows execution and device acceptance. Кросс-сборка не заменяет запуск на Windows.
