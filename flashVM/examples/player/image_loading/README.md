# Child PNG, GIF and JPEG loading

[Русский](README.ru.md) · [Catalog](../../README.md#example-player-image_loading) ·
[Image contract](../../../doc/IMAGE_LOADING.md).

The annotated [C89 host](main.c) runs executable [PNG](png-start.swf),
[GIF](gif-start.swf) or [JPEG](jpeg-start.swf) startup movies. They keep one clip
identity through image arrival, unload and [SWF reload](after.swf). Assertions
check dimensions through ActionGetProperty, removed old child, retained own data,
identity and actual replacement frame execution. A wrong/missing result fails a
bounded wait. The host uses one joined native-byte queue and owner-thread decode.

[make_movies.py](make_movies.py) emits deterministic bytecode, [PNG](picture.png),
[GIF](picture.gif), [JPEG](picture.jpg) and annotated [linked startup](movie.inc).
It needs only Python stdlib; the JPEG is an explicit fixed 8×8 color fixture,
not a runtime Pillow dependency. [PNG AS](png-start.as) and [replacement AS](after.as)
explain the timeline; they are sketches, not compiler/backend acceptance evidence.

From repository root, with C89/C++98, CMake and target SDL2 SDK:

```sh
cmake -S flashVM -B build-image-player -DFVM_BUILD_DESKTOP_FRONTEND=ON \
  -DFVM_BUILD_EXAMPLES=ON -DFVM_BUILD_TESTS=ON -DFVM_PLAYER_SOCKET_HTTP=ON \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-images.defs"
cmake --build build-image-player --target flashvm_player flashvm_example_image_loading
python3 flashVM/examples/player/image_loading/make_movies.py
python3 -m http.server 8000 --bind 127.0.0.1 --directory flashVM/examples/player/image_loading
```

In another terminal:

```sh
./build-image-player/frontend/flashvm_example_image_loading http://127.0.0.1:8000/png-start.swf
./build-image-player/frontend/flashvm_example_image_loading http://127.0.0.1:8000/gif-start.swf
./build-image-player/frontend/flashvm_example_image_loading http://127.0.0.1:8000/jpeg-start.swf
./build-image-player/frontend/flashvm_example_image_loading --embedded http://127.0.0.1:8000/png-start.swf
ctest --test-dir build-image-player -R 'image_loading' --output-on-failure
```

Expected exit 0 and three HTTP requests: startup, selected image, after.swf.
Embedded mode skips startup I/O and uses its explicit URL as the relative base.
No temporary startup file is created. Images briefly appear on a small plain stage;
this is a fast loopback lifecycle example, not an animation or Internet timeout
promise. For a slower visual run use the player with --async-loads --loads 8
--frames 500 --fps 30 --mute and the same startup URL. Native/cross setup is in
[DESKTOP_FRONTEND.md](../../../doc/DESKTOP_FRONTEND.md), network SDK/TLS/trust in
[SOCKET_TRANSPORT.md](../../../doc/SOCKET_TRANSPORT.md). Other CPUs/OS use the same
target with target SDL/codec libraries; foreign linking does not prove execution.
Libcurl uses FVM_PLAYER_HTTP instead of SOCKET_HTTP and requires thread-safe globals.

All stack descriptors, URL and provider userdata live until synchronous return.
The worker only owns bytes; GC pins and image publication stay on the owner.
Joined cancellation precedes unpin/free. Input payloads are immutable and job-owned
until import; decoded bitmap pixels belong to the clip graph afterwards.

| C API | Purpose, ownership and blocking; guide |
| --- | --- |
| [fvm_desktop_background_supported](../../../frontend/common/background.h#L49) | Query worker support without I/O; BACKGROUND_TRANSFER.md. |
| [fvm_desktop_options_init](../../../frontend/common/options.h#L48) | Fill caller scalar descriptor; PLAYER_CONFIGURATION.md. |
| [fvm_desktop_background_loader_init](../../../frontend/common/background.h#L21) | Fill caller provider descriptor without job creation; BACKGROUND_TRANSFER.md. |
| [fvm_desktop_socket_transfer_fetch](../../../frontend/common/socket_http.h#L65) | Native worker adapter with borrowed policy and cooperative stop; SOCKET_TRANSPORT.md. |
| [fvm_desktop_http_transfer_fetch](../../../frontend/common/http.h#L35) | Alternate curl worker adapter with global-safety check; BACKGROUND_TRANSFER.md. |
| [fvm_desktop_run_source_async](../../../frontend/common/background.h#L40) | Borrow descriptors through synchronous owner playback; BACKGROUND_TRANSFER.md. |
| [fvm_desktop_shutdown](../../../frontend/common/player.h#L51) | End SDK after joined jobs/window cleanup; PLAYER_HOST_OWNERSHIP.md. |

Limits: static full-canvas GIF89a only; GIF87a/animation and root image startup
remain unsupported. Source SWF profiles and byte/dimension bounds apply. No synthetic
MovieClipLoader/unload events, progressive streaming decode or physical display/OS
acceptance is implied. The C tutorial is muted; root streaming/audio limits remain.
