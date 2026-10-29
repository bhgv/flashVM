Current player policy / Текущая политика: [single root PCM playback](../../../doc/PLAYER_PCM_STREAMS.md). The standalone C89 example below remains explicitly muted.

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

Use --jit only where available. With --mute the scene plays silently. Since v0.758
this single-PCM fixture also plays without --mute, although three samples are too
short for an audible demonstration; use the [longer PCM example](../pcm_stream/README.md).
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
| [fvm_config_init](../../../include/flashvm/flashvm.h#L70) | Initializes the host-owned configuration; no runtime is allocated. |
| [fvm_runtime_create](../../../include/flashvm/flashvm.h#L74) | Creates an owned runtime from copied configuration; destroy it last. |
| [fvm_context_create](../../../include/flashvm/flashvm.h#L82) | Creates the owner-thread context; it must die before its runtime. |
| [fvm_load_memory](../../../include/flashvm/flashvm.h#L90) | Copies canonical source into the context before flagged import; input remains caller-owned. |
| [fvm_standard_install](../../../include/flashvm/standard.h#L22) | Prepares the standard MovieClip model; no audio device. |
| [fvm_swf_stream_sound_count](../../../include/flashvm/swf_stream_sound.h#L61) | Validates root streams and returns a copied scalar count. |
| [fvm_movieclip_load_swf_ex](../../../include/flashvm/movieclip.h#L216) | Uses explicit [FVM_SWF_IMPORT_MUTE_ROOT_STREAMS](../../../include/flashvm/movieclip.h#L204); requires matching decoded canonical source, clears output on failure. |
| [fvm_gc_pin](../../../include/flashvm/gc.h#L60) / [fvm_gc_unpin](../../../include/flashvm/gc.h#L70) | Host retains/releases the managed clip/library across collection; neither transfers allocation ownership. |
| [fvm_gc_step](../../../include/flashvm/gc.h#L116) | Owner-thread safe-point collection, outside callbacks. |
| [fvm_movieclip_advance](../../../include/flashvm/movieclip.h#L160) | Executes supported root frame behavior while stream controls remain muted. |
| [fvm_context_destroy](../../../include/flashvm/flashvm.h#L86) / [fvm_runtime_destroy](../../../include/flashvm/flashvm.h#L78) | Release managed/native ownership in context-before-runtime order. |

Strict [fvm_movieclip_load_swf](../../../include/flashvm/movieclip.h#L129) still rejects stream controls. Unknown flags, source
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
| `fvm_config_init` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:82](../../../include/flashvm/flashvm.h#L82) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:86](../../../include/flashvm/flashvm.h#L86) |
| `fvm_gc_pin` | function | [include/flashvm/gc.h:60](../../../include/flashvm/gc.h#L60) |
| `fvm_gc_step` | function | [include/flashvm/gc.h:116](../../../include/flashvm/gc.h#L116) |
| `fvm_gc_unpin` | function | [include/flashvm/gc.h:70](../../../include/flashvm/gc.h#L70) |
| `fvm_load_memory` | function | [include/flashvm/flashvm.h:90](../../../include/flashvm/flashvm.h#L90) |
| `fvm_movieclip_advance` | function | [include/flashvm/movieclip.h:160](../../../include/flashvm/movieclip.h#L160) |
| `fvm_movieclip_load_swf` | function | [include/flashvm/movieclip.h:129](../../../include/flashvm/movieclip.h#L129) |
| `fvm_movieclip_load_swf_ex` | function | [include/flashvm/movieclip.h:216](../../../include/flashvm/movieclip.h#L216) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:74](../../../include/flashvm/flashvm.h#L74) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:78](../../../include/flashvm/flashvm.h#L78) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:22](../../../include/flashvm/standard.h#L22) |
| `FVM_SWF_IMPORT_MUTE_ROOT_STREAMS` | macro / define | [include/flashvm/movieclip.h:204](../../../include/flashvm/movieclip.h#L204) |
| `fvm_swf_stream_sound_count` | function | [include/flashvm/swf_stream_sound.h:61](../../../include/flashvm/swf_stream_sound.h#L61) |
<!-- source-links:end -->
