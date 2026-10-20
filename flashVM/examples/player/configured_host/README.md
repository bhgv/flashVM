# Configure playback from a C89 host

[Русский](README.ru.md) · [Catalog](../../README.md#example-player-configured_host) ·
[Detailed API contract](../../../doc/PLAYER_CONFIGURATION.md).

This executable example embeds the shared player through a structure rather than
constructing player CLI arguments. It is useful for a native launcher or an IDE
preview command that can own the main UI thread until playback finishes.
Every nonempty line in [main.c](main.c) is explained. No ActionScript compiler,
linker or JIT is needed for this example.

## Build and run

Requirements: native C89/C++98 toolchain, CMake, graphics-enabled FlashVM profile,
SDL2 2.0.9+ CMake SDK and the optional desktop frontend. From the repository root:

```sh
cmake -S flashVM -B build-player -DFVM_BUILD_DESKTOP_FRONTEND=ON -DFVM_BUILD_EXAMPLES=ON -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs" -DSDL2_DIR=/path/to/native-sdl/lib/cmake/SDL2
cmake --build build-player --target flashvm_example_configured_host --parallel
./build-player/frontend/flashvm_example_configured_host flashVM/examples/player/muted_stream/muted-stream.swf
```

A brief stage window opens and closes after two interpreter ticks; exit status is
zero. The fixture contains root stream controls, so mute is explicitly selected.
No audio endpoint opens. For a display-free native protocol check, prefix the run
with SDL_VIDEODRIVER=dummy and SDL_RENDER_DRIVER=software. This is not a physical
screen test. Build instructions for another OS/CPU and target SDL SDK are in the
[frontend guide](../../../doc/DESKTOP_FRONTEND.md); build directories and generated
metadata must not be reused across targets. No foreign runtime is implied.

## API, ownership and cleanup

| API | Purpose and ownership |
| --- | --- |
| [fvm_desktop_options_init](../../../frontend/common/options.h#L48) | Initialize caller-owned [FVM_DesktopOptions](../../../frontend/common/options.h#L29), including 100-percent authored timing. Null returns zero; no resources are acquired |
| [fvm_desktop_run](../../../frontend/common/player.h#L31) | Validate the complete playlist, then synchronously load/play/clean up. Borrow the immutable paths and array until return. Return 0, 1 or 2 as documented in the contract |
| [FVM_DESKTOP_EXIT_ARGUMENT](../../../frontend/common/player.h#L9) | Return a usage/configuration failure before starting playback |

The program selects interpreter, mute, two ticks and explicit FPS, leaving the
percentage at its required default. It links flashvm_player_host, the same static
implementation used by the CLI. That target supplies FlashVM/SDL and the matching
build-version header; no OS entrypoint is included in the embedding library.
The host owns the options and process arguments. The runner owns temporary VM,
[GC:] roots and native resources; no managed object escapes. Static error strings
must not be freed. Run on the main UI thread; no concurrent or recursive runs.
Final process-wide SDL shutdown belongs to the embedding application.

Limits: synchronous playback, existing standard-stream diagnostics, no widget API,
no routing of other windows' events, no streaming audio decoder/clock and no
complete SWF8 compatibility claim. See [SDL ownership](../../../doc/PLAYER_HOST_OWNERSHIP.md).

[Host declarations](../../../frontend/common/player.h) ·
[Options](../../../frontend/common/options.h) ·
[Implementation](../../../frontend/common/player.c) ·
[Movie fixture](../muted_stream/README.md)
