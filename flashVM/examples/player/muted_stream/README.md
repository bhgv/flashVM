# Explicitly muted root streaming movie

The supplied [SWF8](muted-stream.swf) contains one 320×200 blue-grey frame and
three mono 16-bit PCM samples in SoundStreamHead2/SoundStreamBlock. It is a small
format/import example, not an audible timing demonstration. The standalone
[Python generator](make_movie.py) reproduces its bytes without compilers or test
helpers. [C89 host](main.c), [local bounds](bounds.h),
[import contract](../../../doc/PLAYER_PREFLIGHT.md#explicit-root-stream-muting-v0663).

From the repository root with your SDL player already built:

```sh
python3 flashVM/examples/player/muted_stream/make_movie.py
build/player/frontend/flashvm_player --interpreter --mute --frames 3 \
  flashVM/examples/player/muted_stream/muted-stream.swf
```

Use --jit only where available. With --mute the scene plays silently; without it,
root streaming playback fails explicitly before opening a native window/device.
The latter is intentional until a proper streaming output clock is implemented.
For bounded headless loading, set SDL_VIDEODRIVER=dummy and --fps 1000. That is not
physical video/audio acceptance.

The C89 host uses no SDL and opens no device. Its file reader uses platform
C-runtime filename encoding; the SDL player retains its UTF-8 file adapter. Build it against the same VM source:

```sh
cmake -S flashVM -B build/vm -DFVM_BUILD_EXAMPLES=ON
cmake --build build/vm --target flashvm_examples_player_muted_stream_main
build/vm/flashvm_examples_player_muted_stream_main \
  flashVM/examples/player/muted_stream/muted-stream.swf
```

Expected output: `streams=1 advanced=1 muted=1`. The host reads a stable seekable
file, keeps input alive through both load calls, then frees it. It pins the returned
clip/library, runs a safe GC step and one advance, unpins, destroys the context,
then the runtime. Errors unwind the same resources. Exit 0 success, 1 I/O/API
failure, 2 usage. It does not render; the player command above supplies the SDL
presentation. [Native/cross build guide](../../../../BUILDING.md) applies; a cross
SDK must match target OS/CPU, and the resulting host must run there.

| API | Purpose and ownership |
| --- | --- |
| [fvm_config_init](../../../include/flashvm/flashvm.h#L62) | Initializes the host-owned configuration; no runtime is allocated. |
| [fvm_runtime_create](../../../include/flashvm/flashvm.h#L64) | Creates an owned runtime from copied configuration; destroy it last. |
| [fvm_context_create](../../../include/flashvm/flashvm.h#L68) | Creates the owner-thread context; it must die before its runtime. |
| [fvm_load_memory](../../../include/flashvm/flashvm.h#L72) | Copies canonical source into the context before flagged import; input remains caller-owned. |
| [fvm_standard_install](../../../include/flashvm/standard.h#L20) | Prepares the standard MovieClip model; no audio device. |
| [fvm_swf_stream_sound_count](../../../include/flashvm/swf_stream_sound.h#L53) | Validates root streams and returns a copied scalar count. |
| [fvm_movieclip_load_swf_ex](../../../include/flashvm/movieclip.h#L125) | Uses explicit [FVM_SWF_IMPORT_MUTE_ROOT_STREAMS](../../../include/flashvm/movieclip.h#L115); requires matching decoded canonical source, clears output on failure. |
| [fvm_gc_pin](../../../include/flashvm/gc.h#L48) / [fvm_gc_unpin](../../../include/flashvm/gc.h#L54) | Host retains/releases the managed clip/library across collection; neither transfers allocation ownership. |
| [fvm_gc_step](../../../include/flashvm/gc.h#L90) | Owner-thread safe-point collection, outside callbacks. |
| [fvm_movieclip_advance](../../../include/flashvm/movieclip.h#L99) | Executes supported root frame behavior while stream controls remain muted. |
| [fvm_context_destroy](../../../include/flashvm/flashvm.h#L70) / [fvm_runtime_destroy](../../../include/flashvm/flashvm.h#L66) | Release managed/native ownership in context-before-runtime order. |

Strict [fvm_movieclip_load_swf](../../../include/flashvm/movieclip.h#L76) still rejects stream controls. Unknown flags, source
mismatch, malformed root records and sprite streams are not silently ignored.
Compressed stream bytes remain opaque; this example does not add MP3 decoding,
stream mixing, A/V sync, video codecs or complete Flash sound semantics. Event
sounds retain their separate host callback policy; this fixture contains none.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:62](../../../include/flashvm/flashvm.h#L62) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:68](../../../include/flashvm/flashvm.h#L68) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_gc_pin` | function | [include/flashvm/gc.h:48](../../../include/flashvm/gc.h#L48) |
| `fvm_gc_step` | function | [include/flashvm/gc.h:90](../../../include/flashvm/gc.h#L90) |
| `fvm_gc_unpin` | function | [include/flashvm/gc.h:54](../../../include/flashvm/gc.h#L54) |
| `fvm_load_memory` | function | [include/flashvm/flashvm.h:72](../../../include/flashvm/flashvm.h#L72) |
| `fvm_movieclip_advance` | function | [include/flashvm/movieclip.h:99](../../../include/flashvm/movieclip.h#L99) |
| `fvm_movieclip_load_swf` | function | [include/flashvm/movieclip.h:76](../../../include/flashvm/movieclip.h#L76) |
| `fvm_movieclip_load_swf_ex` | function | [include/flashvm/movieclip.h:125](../../../include/flashvm/movieclip.h#L125) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../../../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../../../include/flashvm/flashvm.h#L66) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:20](../../../include/flashvm/standard.h#L20) |
| `FVM_SWF_IMPORT_MUTE_ROOT_STREAMS` | macro / define | [include/flashvm/movieclip.h:115](../../../include/flashvm/movieclip.h#L115) |
| `fvm_swf_stream_sound_count` | function | [include/flashvm/swf_stream_sound.h:53](../../../include/flashvm/swf_stream_sound.h#L53) |
<!-- source-links:end -->
