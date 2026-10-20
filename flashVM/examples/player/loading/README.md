# Embedded startup, child movie and loadVariables

[Русский](README.ru.md) · [Catalog](../../README.md#example-player-loading) ·
[Source/loading contract](../../../doc/PLAYER_SOURCES.md).

This runnable C89 example embeds the startup movie and loads a child plus two form
resources. The movie checks the resulting strings and throws on a wrong value.
Every nonempty C/header/AS line is explained. start.as and nested/child.as are
annotated source sketches separated into timeline frames; make_movies.py emits
the actual matching SWF bytecode independently of unfinished compiler backends.
Do not compile start.as as one linear frame and expect the deferred checks to work.

## Files and scheduling

main.c links startup.h; no startup SWF is opened by the memory runner. Frame 1
creates child, requests nested/child.swf into it, and root-vars.txt into the root.
Both are delivered after the frame returns. Frame 2 checks greeting and advances
the new child; its this.loadVariables requests nested/child-vars.txt. Frame 3
checks child.answer. The child stops after requesting once. The host closes after
three ticks. root-vars.txt uses plus for a space; child-vars.txt uses percent escapes.
The generated start.swf can also be run directly from disk or a web server.

## Native and cross builds

Enable the optional frontend/examples with C89/C++98, CMake, a graphics DEFS profile
and SDL2 2.0.9+ target SDK as in the [build guide](../../../doc/DESKTOP_FRONTEND.md).

```sh
cmake --build build-player --target flashvm_example_loading
cp -R flashVM/examples/player/loading/nested build-player/frontend/
cp flashVM/examples/player/loading/root-vars.txt build-player/frontend/
./build-player/frontend/flashvm_example_loading
```

Memory playback defaults to SDL's executable-directory discovery, independently of
process CWD. On a restricted host where that discovery is unavailable, pass a full
synthetic source path: it need not exist, but its directory must hold the resources.
This explicit-origin run uses the shipped files and is also the CTest form:

```sh
./build-player/frontend/flashvm_example_loading "$PWD/flashVM/examples/player/loading/embedded.swf"
./build-player/frontend/flashvm_player --once --mute --loads 8 flashVM/examples/player/loading/start.swf
```

Both close normally with status 0; no root/child data assertion fails. To rebuild
fixtures use python3 flashVM/examples/player/loading/make_movies.py. For HTTP,
configure FVM_PLAYER_HTTP=ON with a target libcurl SDK, then:

```sh
python3 -m http.server 8000 --directory flashVM/examples/player/loading
# In another terminal:
./build-player/frontend/flashvm_player --once --mute --loads 8 http://127.0.0.1:8000/start.swf
```

For cross builds use separate toolchains/build directories/target SDL (and curl
when enabled). Shared host builds require PIC dependencies on ELF/Mach-O. See
[source/module setup](../../../doc/PLAYER_SOURCES.md); cross-linking is not runtime
acceptance. Dummy tests use SDL_VIDEODRIVER=dummy and SDL_RENDER_DRIVER=software.

## API and ownership

| API | Purpose |
| --- | --- |
| [fvm_desktop_options_init](../../../frontend/common/options.h#L48) | Initialize caller-owned scalar settings |
| [fvm_desktop_run_memory](../../../frontend/common/player.h#L42) | Borrow exact startup bytes/name until synchronous return |
| [FVM_DesktopOptions.max_loads](../../../frontend/common/options.h#L35) | Bound startup plus all deferred movie/form requests |
| loadMovie / MovieClip.loadVariables | Queue copied requests; do not fetch from the active callback |
| [fvm_desktop_shutdown](../../../frontend/common/player.h#L49) | End this standalone application's global session after all player cleanup |

[GC:] The runner pins queued targets until delivery, preserves child identity and
placement during replacement, and releases request roots on error/close. No managed
pointer escapes. Run on the native UI thread without recursive/concurrent playback.
The parent survives child and variable loading. Form assignment uses ordered public
setters; completed setter effects cannot be rolled back. The AS sketches are not a
claim of full language/compiler or Flash loading conformance. POST, progressive
loading and child root streaming sound remain outside this tested subset.

[Host declarations](../../../frontend/common/player.h) ·
[Core loading APIs](../../../include/flashvm/movieclip.h) ·
[Implementation](../../../frontend/common/player.c) ·
[Core implementation](../../../src/standard/movieclip_loading.cpp)
