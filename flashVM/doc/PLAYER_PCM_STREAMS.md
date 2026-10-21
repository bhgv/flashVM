# Single root PCM stream in the SDL player

Since v0.758 the actual desktop player accepts one validated root SoundStreamHead/Head2 segment in PCM_NATIVE or PCM_LE format, with no root StartSound events. It converts native source samples to 48000 Hz interleaved stereo S16 and feeds the real SDL queue. Unsupported codecs, multiple header segments and mixed event/stream movies fail explicitly during preflight, before window/device creation. --mute continues to validate and omit root streaming controls without opening audio. Strict core scene import remains strict; the player uses the exact-source root-stream omission flag and separately snapshots the validated source before starting its worker.

## Build and run

Use a standalone FlashVM build, its documented SDL2 static SDK and a rendering profile from [player builds](PLAYER_TEST_BUILDS.md); the core alone still needs no SDL. Enable FVM_ENABLE_SDL_AUDIO and FVM_BUILD_DESKTOP_FRONTEND. After building flashvm_player:

```sh
python3 flashVM/examples/player/pcm_stream/make_movie.py
build/player/frontend/flashvm_player --interpreter --once flashVM/examples/player/pcm_stream/pcm-stream.swf
build/player/frontend/flashvm_player --jit --once flashVM/examples/player/pcm_stream/pcm-stream.swf
```

Use JIT only on an available backend. The [reproducible example](../examples/player/pcm_stream/README.md) changes its background and tone after one second. It is a runnable Python-generated SWF, not a compiler implementation. The structured fvm_desktop_run and fvm_desktop_run_memory entry points use the same player path; no CLI-only state or ABI fields were added. Existing source-base and load policy is unchanged. Replacement/unload still uses the strict existing import policy; child/sprite streams are unsupported.

## Owner and worker

The owner imports/pins the root and calls [fvm_movieclip_get_playback_info](../include/flashvm/movieclip.h#L83) at host service points. Selected-but-not-entered frames authorize nothing. The scalar entered frame and local play flag are published under the audio mutex; no context, clip or managed value is read by the worker. The worker owns the prepared snapshot, converter position and retry buffer, and calls [fvm_swf_stream_output_pump_until](../include/flashvm/swf_stream_output.h#L64) at most FVM_DESKTOP_AUDIO_PUMPS times per service iteration. The SDL backend copies accepted PCM; it never calls into VM/GC. Queue targets use the actual obtained driver quantum. Short authorized blocks and EOF resume with a nonempty queue even below the usual reserve, avoiding a startup deadlock when a frame contains little audio.

Host/session/background suspension and the clip's stop flag both pause the queue and preserve accepted bytes. play resumes authorized data. StopSounds pauses, clears and cancels the stream until a detected navigation reset. There is no stream stop-release ramp. Pending, backward or skipped navigation invokes [fvm_swf_stream_output_seek_frame](../include/flashvm/swf_stream_output.h#L83) under the same mutex, clearing old queued/pending bytes and starting phase zero at the first block at/after the selected frame. Gaps hold a later block; they do not synthesize silence. Source EOF and drained queue are distinct.

Observation is not a complete navigation history: a same-frame goto that is selected and entered entirely between two host observations may be invisible in v0.758. Clock drift/drop/stretch, multiple headers, stream+event mixing, MP3/ADPCM stream decoding, video synchronization and reference Flash seek semantics remain open. No claim of all Flash streaming behavior is made. Root unload detaches delivery, joins the worker and closes snapshot/queue/device ownership before the context is destroyed; GC runs only on the owner at normal safe points.

## Internal host API and ownership

These functions are in [audio.h](../frontend/common/audio.h), implemented in [audio.c](../frontend/common/audio.c). They are private frontend helpers, not a new stable plugin ABI. The existing public player C entry points retain their signatures.

| Operation | Contract |
| --- | --- |
| fvm_desktop_audio_validate | Owner-thread root count/format/event policy; OK for no streams, UNSUPPORTED for unsupported combinations, malformed stream status propagates. No native device or allocation ownership is acquired. |
| fvm_desktop_audio_open_paused | Acquire snapshot/converter/FIFO/device/mutex/worker and stop binding only for actual audio. Supplied pause is 0/1. Errors propagate; caller must close even after partial acquisition. Snapshot is independent of VM lifetime, but owner unregisters stop binding before context destruction. |
| fvm_desktop_audio_timeline | Owner reads retained same-context clip and publishes copied policy under mutex. VM query, seek and backend errors propagate. Navigation clear/pause has staged external effects; it is not rollback. No worker VM call. |
| fvm_desktop_audio_stream_info | Stream-only copied [FVM_SWFStreamOutputInfo](../include/flashvm/swf_stream_output.h#L11) under mutex; invalid input/error preserves descriptor. May wait for worker lock/backend observation. It reports FIFO dequeue position, not DAC/movie synchronization. |
| fvm_desktop_audio_pause / fvm_desktop_audio_tick_paused | Preserve host pause independently of local play; pause never clears bytes, tick polls latched worker error. Both require serial host use; worker is internally serialized. |
| fvm_desktop_audio_clear / fvm_desktop_audio_close | Owner detaches binding/callback, joins worker, closes snapshot and real device, then destroys mutex. Clear retains cumulative diagnostics. Never destroy a context or free userdata before detachment/quiescence. |

## Evidence and limits

stream_worker.cpp exercises actual SDL dummy queue output with a real pinned MovieClip: initial hold, frame gaps, slow owner, stop/play, GC, EOF, StopSounds, backward navigation, host suspension and joined cleanup, in interpreter/JIT selections. stream_audio.py independently creates FWS/CWS with actual AVM1 frame actions and compares every accepted PCM sample from the real CLI trace against 9600 exact stereo frames. stream_preflight.py checks supported and unsupported/malformed/muted combinations with deliberately unavailable drivers on rejected paths. Dummy audio validates queue/control behavior, not audible hardware, other operating systems or other CPUs. FVM_AUDIO_TRACE explicitly opts into bounded RAM capture and file output on close; it records accepted blocks, including bytes later discarded by a clear, not speaker playback. All roadmap stages remain open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_movieclip_get_playback_info` | function | [include/flashvm/movieclip.h:83](../include/flashvm/movieclip.h#L83) |
| `fvm_swf_stream_output_pump_until` | function | [include/flashvm/swf_stream_output.h:64](../include/flashvm/swf_stream_output.h#L64) |
| `fvm_swf_stream_output_seek_frame` | function | [include/flashvm/swf_stream_output.h:83](../include/flashvm/swf_stream_output.h#L83) |
| `FVM_SWFStreamOutputInfo` | struct | [include/flashvm/swf_stream_output.h:11](../include/flashvm/swf_stream_output.h#L11) |
<!-- source-links:end -->
