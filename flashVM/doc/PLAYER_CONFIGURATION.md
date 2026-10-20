# Structured player configuration — v0.672

[Русский](PLAYER_CONFIGURATION.ru.md).

A C89/C++98 application can link the optional CMake target `flashvm_player_host`
and call [fvm_desktop_run](../frontend/common/player.h#L38) without constructing a command line. This static target
contains the shared scheduler, scene/VM integration, input, renderer and audio
adapters. It links FlashVM and the selected SDL SDK transitively. The console
entrypoint [fvm_desktop_main](../frontend/common/player.h#L60) parses arguments and invokes the same runner.
This is an internal frontend contract, not a new stable VM ABI or an installed SDK.

## API and lifetime

Include frontend/common/player.h; the target exports that include directory.
[fvm_desktop_options_init](../frontend/common/options.h#L48) accepts required [FVM_DesktopOptions](../frontend/common/options.h#L29) storage, returns
one on success and zero for null, and sets automatic execution, HIGH quality,
authored timing at 100 percent, no limits/flags and an empty playlist. It does
not allocate or open anything. Select at least one file before calling run.

[fvm_desktop_options_validate](../frontend/common/options.h#L51) returns one for valid settings, otherwise zero.
The optional error pointer receives null on success or a static diagnostic on
failure; callers must not free it. Validation does not modify the options, open
files, initialize SDL/VM, allocate, print, or change the process locale.

Validation checks execution mode, quality, finite timing ranges, boolean flags,
once/frame-limit conflicts, playlist size arithmetic and every nonempty path.
FPS is either zero (authored rate) or 0.1..1000; percent is 0.1..10000 and must
remain 100 when explicit FPS is selected. Enum values outside the declared
choices and flags other than zero/one are rejected. Counts must describe actual
accessible storage; C cannot validate arbitrary addresses or determine its size.

[fvm_desktop_run](../frontend/common/player.h#L38) returns [FVM_DESKTOP_EXIT_ARGUMENT](../frontend/common/player.h#L16) (2) for malformed configuration
before opening even the first file. It copies scalar settings and borrows the
playlist array and NUL-terminated path strings until synchronous return; keep
all of them immutable and accessible. Empty/null later paths reject the entire
playlist before startup. It returns [FVM_DESKTOP_EXIT_OK](../frontend/common/player.h#L14) (0) on normal completion
or user close, and [FVM_DESKTOP_EXIT_PLAYBACK](../frontend/common/player.h#L15) (1) for file, VM or device failure.
The optional error is null on success, a detailed static validation diagnostic
on invalid input, or a generic playback diagnostic; existing playback errors,
trace and stats still use standard streams. This is not a diagnostic callback API.

Run on the native UI/VM owner thread. Concurrent or recursive runs are unsupported;
the runner owns its temporary VM, collector roots and native view, and releases
them before return. It does not expose managed objects. The host retains final
SDL-session ownership; see [SDL ownership](PLAYER_HOST_OWNERSHIP.md). It is a
synchronous player, not a widget/tick API or a foreign-window event router.
A multi-file playlist runs each movie for its authored frame count unless an
explicit frame limit is given. Close/Escape ends the entire playlist. File and
device failures stop playback at that movie; completed movies are not rolled back.

## Build and example

Enable FVM_BUILD_DESKTOP_FRONTEND and FVM_BUILD_EXAMPLES with a graphics-capable
DEFS profile and an SDL2 2.0.9+ target SDK. Follow the [native/cross guide](DESKTOP_FRONTEND.md)
for dependency and toolchain setup; the same target builds for each selected
OS/CPU. Build host tools natively and use a separate directory and target SDL
SDK for each cross build. Linking a foreign executable does not test its runtime.

```sh
cmake -S flashVM -B build-player -DFVM_BUILD_DESKTOP_FRONTEND=ON -DFVM_BUILD_EXAMPLES=ON -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs" -DSDL2_DIR=/path/to/native-sdl/lib/cmake/SDL2
cmake --build build-player --target flashvm_player_host flashvm_example_configured_host --parallel
SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software ./build-player/frontend/flashvm_example_configured_host flashVM/examples/player/muted_stream/muted-stream.swf
ctest --test-dir build-player -R 'frontend_configuration|frontend_build_info|example_configured_host' --output-on-failure
```

An application added to the same CMake build links `flashvm_player_host`; it does
not compile an OS main.c or call the CLI parser. Generated version metadata stays
in the correct target's build tree. Android keeps the existing SDLActivity entry
and links the host library as PIC; the standalone desktop example is omitted.

The [annotated C89 example](../examples/player/configured_host/README.md) plays
two muted interpreter ticks through direct configuration. The C89 configuration
fixture checks rejected families and unchanged input. The C++98 fixture links
all C entrypoints and rejects a null second path before a nonexistent first file
or nonexistent device driver can be accessed. Native dummy rendering is a host
protocol check, not physical display/audio or complete platform acceptance.

[Options](../frontend/common/options.h) · [Runner](../frontend/common/player.h) ·
[Implementation](../frontend/common/player.c) · [Configuration tests](../frontend/tests/configuration.c) ·
[C++98 tests](../frontend/tests/build_info.cpp) · [Roadmap](ROADMAP_STATUS.md)

## v0.672 validation

65 selected native tests passed on Linux x86-64: 52 frontend/player/capture,
metadata and example checks, plus 13 related scene, sound, timer and GC checks.
SDL 2.30.12 used dummy video/software rendering; the SDL audio bridge was disabled.
The structured example actually loaded the supplied SWF and completed two ticks.
This was a selected regression run, not the entire project suite.

Full players and the new C89 configured-host example also linked for ARM64 Linux
(musl), RISC-V64 Linux (musl) and x86-64 Windows (GNU), with target-specific static
SDL SDKs and the existing sandbox-adapted Zig 0.13 toolchain. ELF/PE machine types
were checked. Foreign executables were not run; this does not certify a stock SDK,
physical devices, macOS or Android acceptance. All nine roadmap stages remain open.

[SWF sources / Источники SWF](PLAYER_SOURCES.md).
