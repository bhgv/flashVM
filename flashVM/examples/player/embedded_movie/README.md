# SWF linked into an executable

[Русский](README.ru.md) · [Catalog](../../README.md#example-player-embedded_movie) ·
[Memory API contract](../../../doc/PLAYER_SOURCES.md).

This complete C89 example plays an immutable linked SWF8 byte array. No SWF file
or CLI argument is needed at runtime. Suitable for an application startup movie,
a preview resource or code later packaged in a host library. Every nonempty C/header
line is explained. movie.h contains the exact muted_stream fixture bytes.

Requirements: C89/C++98, CMake, SDL2 2.0.9+ CMake SDK, graphics-enabled DEFS profile,
FVM_BUILD_DESKTOP_FRONTEND=ON and FVM_BUILD_EXAMPLES=ON. Follow the
[configuration build instructions](../../../doc/PLAYER_CONFIGURATION.md), then:

```sh
cmake --build build-player --target flashvm_example_embedded_movie
./build-player/frontend/flashvm_example_embedded_movie
```

A small window closes after two muted interpreter ticks, returning zero.
For a display-free protocol run set SDL_VIDEODRIVER=dummy and SDL_RENDER_DRIVER=software.
This does not certify a physical display. Use a separate target SDL/toolchain/build
directory for cross compilation as described in the frontend guide.

| API | Purpose |
| --- | --- |
| fvm_desktop_options_init | Shared defaults; caller owns structure |
| fvm_desktop_run_memory | Borrow binary bytes/name until synchronous return; exact sizeof, no strlen |
| SDL_Quit | Standalone host ends its own complete SDL session after player resources are released |

[main.c](main.c) selects mute because the fixture includes root stream controls.
[movie.h](movie.h) remains owned by the executable; never free it. [GC:] The runner
owns and releases VM roots; no managed value escapes. Run on the native UI thread,
without parallel/reentrant calls. Existing diagnostics use standard streams.
Limits remain those of the current SWF importer and renderer; no streaming decoder,
widget integration or complete Flash compatibility is implied.

[Declarations](../../../frontend/common/player.h) ·
[Implementation](../../../frontend/common/player.c) ·
[Fixture documentation](../muted_stream/README.md)
