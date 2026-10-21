# MP3 asset export after VM teardown / Экспорт MP3 после уничтожения VM

Executable strict-C89 example for extraction, offline conversion and native worker
preparation. [Current MP3 contract EN](../../../doc/MP3_AUDIO.md) /
[Контракт RU](../../../doc/MP3_AUDIO.ru.md). Input is an actual SWF4–8 file; the
example never invokes FFmpeg, SDL, a network service or an ActionScript compiler.

## Modes and ownership / Режимы и владение

| Command | Prepared source and output |
| --- | --- |
| INPUT.swf OUTPUT.pcm | First root MP3 StartSound: prepared voice → native mixer adoption after VM teardown, authored styles, native-rate S16 |
| INPUT.swf OUTPUT.pcm --stream | First root MP3 segment: continuous MPEG history, one initial latency trim, native source-rate S16 |

[main.c](main.c) bounds the source file, loads/copies it into a context, releases
the caller buffer, prepares independent native ownership and copies effective
rate/channels. Context and runtime are destroyed **before** output is opened.
After teardown, event mode creates an explicit same-rate mixer and transfers the
prepared voice using [fvm_swf_sound_mixer_submit_prepared](../../../include/flashvm/swf_sound_mixer.h#L78). Success clears the old
voice handle; errors preserve it for cleanup. Conversion policy permits effective
MPEG rates, but matching source/output geometry causes no resampling. The host
bounds mixer output to frames_per_play times play_count, never an infinite silence
tail. [fvm_swf_sound_mixer_stop_id](../../../include/flashvm/swf_sound_mixer.h#L110) ends future same-ID composition without touching
a device.

The export loop uses a caller-owned aligned buffer; it never reads a VM/GC pointer,
creates a worker/device, resamples or synthesizes timeline gaps. Source snapshot
outlives all readers and closes after they stop. First StartSound must be a playable
start control, not a stop; first selected resource/segment must use MP3.

[example_constants.h](example_constants.h) sets separate host limits: 16 MiB input,
32 MiB runtime/native snapshot preparation, 32 million exported frames and 1024
frames per copy. These bounds do not certify process-wide RSS. In stream mode the
native ceiling covers object/index plus simultaneous encoded and decoded bytes;
stack, file buffering, allocator metadata and VM ownership are separate. Event
voices retain their existing payload/envelope runtime budget.

PCM byte order follows the host; the file has no WAV header. Output contains
exactly the selected native frames at the printed effective rate/channel count.
File output replaces the caller-selected path. Short writes/close errors are
terminal and can leave a partial file; no atomic queue retry is claimed. Cleanup
releases partial input/context/native/output acquisition on every exit.

Пример создаёт native ownership на owner-потоке, затем полностью уничтожает VM.
Native reader/voice живёт до конца FILE export. Нет hidden clock, playback worker,
SDL или source borrow. Отдельные лимиты — политика хоста, не ограничения SWF.
При ошибке файл может быть частично записан; повторное выполнение — новый экспорт.

## Build and run / Сборка и запуск

```sh
cmake -S flashVM -B build-mp3 \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/minimal.defs" \
  -DFVM_BUILD_EXAMPLES=ON -DFVM_BUILD_TESTS=ON
cmake --build build-mp3 --target flashvm_examples_multimedia_swf_mp3_main
./build-mp3/flashvm_examples_multimedia_swf_mp3_main
./build-mp3/flashvm_examples_multimedia_swf_mp3_main event.swf voice.pcm
./build-mp3/flashvm_examples_multimedia_swf_mp3_main stream.swf stream.pcm --stream
python3 flashVM/scripts/check_swf_mp3_example.py ./build-mp3/flashvm_examples_multimedia_swf_mp3_main
```

C89/C++98, CMake and bundled static dependencies are sufficient; NO_MEDIA is
supported. No arguments prints usage without file/VM side effects. Invalid CLI,
missing/oversized/non-seekable input, unsupported source and file errors fail.
The checker builds real FWS/CWS fixtures for six MPEG geometries and verifies
independent golden positions, exact output lengths, replacement and terminal
errors. If FFmpeg is available it additionally compares every exported sample;
FFmpeg is a test oracle only, never a runtime dependency.

## APIs in main.c / Использованные API

Detailed codec/source contracts: [EN](../../../doc/MP3_AUDIO.md) / [RU](../../../doc/MP3_AUDIO.ru.md).

| API | Purpose and lifetime/execution |
| --- | --- |
| [fvm_config_init](../../../include/flashvm/flashvm.h#L62) | Initialize caller policy synchronously before any runtime owner |
| [fvm_runtime_create](../../../include/flashvm/flashvm.h#L64) | Acquire bounded source runtime; release after context teardown |
| [fvm_context_create](../../../include/flashvm/flashvm.h#L68) | Allocate source context borrowing runtime; owner-thread preparation only |
| [fvm_load_memory](../../../include/flashvm/flashvm.h#L72) | Validate/copy input SWF; caller buffer is freed immediately on success |
| [fvm_swf_stream_sound_at](../../../include/flashvm/swf_stream_sound.h#L55) | Inspect first root codec before preparation; borrowed metadata remains owner-side |
| [fvm_swf_stream_sound_snapshot_open_pcm_bounded](../../../include/flashvm/swf_stream_sound.h#L102) | Decode and validate complete MP3 history under peak ceiling; publish independent immutable PCM |
| [fvm_swf_stream_sound_snapshot_info](../../../include/flashvm/swf_stream_sound.h#L127) | Copy effective native geometry without allocation/VM/callback |
| [fvm_swf_stream_pcm_reader_init](../../../include/flashvm/swf_stream_pcm.h#L40) | Initialize caller cursor borrowing snapshot; no new owner/allocation |
| [fvm_swf_sound_event_at](../../../include/flashvm/swf_sound.h#L62) | Resolve first root StartSound synchronously before VM destruction |
| [fvm_swf_sound_info](../../../include/flashvm/swf_sound.h#L134) | Inspect referenced codec and effective MPEG rate/channels; borrowed bytes stay owner-side |
| [fvm_swf_sound_voice_open](../../../include/flashvm/swf_sound.h#L82) | Copy/decode authored native interval and envelope; failed acquisition clears output |
| [fvm_swf_sound_voice_info](../../../include/flashvm/swf_sound.h#L114) | Copy native rate/channels/progress without source VM access |
| [fvm_context_destroy](../../../include/flashvm/flashvm.h#L70) | End managed context after independent source preparation |
| [fvm_runtime_destroy](../../../include/flashvm/flashvm.h#L66) | End source runtime before native output; no reader borrows it |
| [fvm_swf_stream_pcm_reader_next](../../../include/flashvm/swf_stream_pcm.h#L48) | Copy bounded normalized stream chunks; native EOF is NOT_FOUND |
| [fvm_swf_sound_mixer_create_configured](../../../include/flashvm/swf_sound_mixer.h#L48) | Allocate an explicit same-rate one-slot native mixer after VM teardown; no source/context reference |
| [fvm_swf_sound_mixer_submit_prepared](../../../include/flashvm/swf_sound_mixer.h#L78) | Adopt prepared voice without allocation/VM; clear handle only on success, preserve it on failure |
| [fvm_swf_sound_mixer_read](../../../include/flashvm/swf_sound_mixer.h#L114) | Produce only explicitly authorized finite frames; same-geometry native PCM, no allocation/callback |
| [fvm_swf_sound_mixer_stop_id](../../../include/flashvm/swf_sound_mixer.h#L110) | Apply native immediate/DECLICK same-ID stop to future composition; no queued-PCM cancellation |
| [fvm_swf_sound_mixer_close](../../../include/flashvm/swf_sound_mixer.h#L133) | Release adopted voices after export quiescence; caller no longer closes a transferred handle |
| [fvm_swf_sound_voice_close](../../../include/flashvm/swf_sound.h#L128) | Null-safe native voice release after reads stop; no device/VM callback |
| [fvm_swf_stream_sound_snapshot_close](../../../include/flashvm/swf_stream_sound.h#L160) | Null-safe native stream release after reader quiescence |

This is a native exporter, not a movie scheduler. Negative initial latency,
free-format bitrate, reference MP3 seeking/read-ahead, root stream/event mixing,
child streams and A/V synchronization remain open; no physical/foreign acceptance
is inferred. / Полная SWF≤8 совместимость и приёмка устройств не объявляются.

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
| `fvm_load_memory` | function | [include/flashvm/flashvm.h:72](../../../include/flashvm/flashvm.h#L72) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../../../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../../../include/flashvm/flashvm.h#L66) |
| `fvm_swf_sound_event_at` | function | [include/flashvm/swf_sound.h:62](../../../include/flashvm/swf_sound.h#L62) |
| `fvm_swf_sound_info` | function | [include/flashvm/swf_sound.h:134](../../../include/flashvm/swf_sound.h#L134) |
| `fvm_swf_sound_mixer_close` | function | [include/flashvm/swf_sound_mixer.h:133](../../../include/flashvm/swf_sound_mixer.h#L133) |
| `fvm_swf_sound_mixer_create_configured` | function | [include/flashvm/swf_sound_mixer.h:48](../../../include/flashvm/swf_sound_mixer.h#L48) |
| `fvm_swf_sound_mixer_read` | function | [include/flashvm/swf_sound_mixer.h:114](../../../include/flashvm/swf_sound_mixer.h#L114) |
| `fvm_swf_sound_mixer_stop_id` | function | [include/flashvm/swf_sound_mixer.h:110](../../../include/flashvm/swf_sound_mixer.h#L110) |
| `fvm_swf_sound_mixer_submit_prepared` | function | [include/flashvm/swf_sound_mixer.h:78](../../../include/flashvm/swf_sound_mixer.h#L78) |
| `fvm_swf_sound_voice_close` | function | [include/flashvm/swf_sound.h:128](../../../include/flashvm/swf_sound.h#L128) |
| `fvm_swf_sound_voice_info` | function | [include/flashvm/swf_sound.h:114](../../../include/flashvm/swf_sound.h#L114) |
| `fvm_swf_sound_voice_open` | function | [include/flashvm/swf_sound.h:82](../../../include/flashvm/swf_sound.h#L82) |
| `fvm_swf_stream_pcm_reader_init` | function | [include/flashvm/swf_stream_pcm.h:40](../../../include/flashvm/swf_stream_pcm.h#L40) |
| `fvm_swf_stream_pcm_reader_next` | function | [include/flashvm/swf_stream_pcm.h:48](../../../include/flashvm/swf_stream_pcm.h#L48) |
| `fvm_swf_stream_sound_at` | function | [include/flashvm/swf_stream_sound.h:55](../../../include/flashvm/swf_stream_sound.h#L55) |
| `fvm_swf_stream_sound_snapshot_close` | function | [include/flashvm/swf_stream_sound.h:160](../../../include/flashvm/swf_stream_sound.h#L160) |
| `fvm_swf_stream_sound_snapshot_info` | function | [include/flashvm/swf_stream_sound.h:127](../../../include/flashvm/swf_stream_sound.h#L127) |
| `fvm_swf_stream_sound_snapshot_open_pcm_bounded` | function | [include/flashvm/swf_stream_sound.h:102](../../../include/flashvm/swf_stream_sound.h#L102) |
<!-- source-links:end -->
