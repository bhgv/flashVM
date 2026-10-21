# SWF stream + event mixer / Потоковый PCM и событийные голоса

This C89 tutorial reads both source kinds from **one actual embedded SWF8** and
mixes them after destroying the source VM. Use it as a native file-export or IDE
integration starting point. [Detailed EN contract](../../../doc/STREAM_EVENT_MIX.md) /
[Контракт RU](../../../doc/STREAM_EVENT_MIX.ru.md), [functional roadmap](../../../doc/ROADMAP.md). Queue composition: [EN](../../../doc/MIX_QUEUE.md) / [RU](../../../doc/MIX_QUEUE.ru.md).

## Input and output / Вход и выход

The literal movie in [main.c](main.c) contains one 44100 Hz mono PCM stream with
eight samples and two independent PCM event resources: +30000 for eight samples,
-30000 for twelve samples. Both StartSound events belong to its only frame. The
example explicitly schedules them together. It prepares a bounded stream snapshot
and a two-voice mixer on the owner, then destroys context/runtime before opening
the output file. All source bytes and voice clocks in the loop are independent
native storage; no GC pointer, hidden worker, SDL or external encoder is needed.

Both paths convert to 48000 Hz stereo. Four-frame caller buffers receive stream
PCM, then add every voice in wide precision before a single S16 saturation. The
first result is 30000; clipping stream+positive voice before adding the negative
voice would corrupt it. At stream EOF explicit zeros become the base for the
longer event tail. The host deliberately exports 16 frames (64 bytes), including
two final silence frames. This is a finite export policy, not automatic Flash
movie/device synchronization or a new desktop mixed-playback mode.

Пример готовит оба источника из одного SWF, уничтожает исходную VM и пишет ровно
16 stereo кадров S16 на 48000 Hz. Нулевой базой после stream EOF управляет хост.
Голоса не обрезаются до сложения с базой. Ни SDL, ни декодер, ни компилятор AS для
запуска не нужны. Константы [example_constants.h](example_constants.h) определяют
геометрию, блок и конечную длительность; 64 KiB ограничивают только stream snapshot,
а не весь процесс/оба event-источника.

## Build and run / Сборка и запуск

From the repository root / из корня репозитория:

```sh
cmake -S flashVM -B build-stream-mix \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/minimal.defs" \
  -DFVM_BUILD_EXAMPLES=ON -DFVM_BUILD_TESTS=ON
cmake --build build-stream-mix --target flashvm_examples_multimedia_swf_stream_mix_main
./build-stream-mix/flashvm_examples_multimedia_swf_stream_mix_main
./build-stream-mix/flashvm_examples_multimedia_swf_stream_mix_main mixed.pcm
./build-stream-mix/flashvm_examples_multimedia_swf_stream_mix_main queued.pcm --queue
python3 flashVM/scripts/check_swf_stream_mix_example.py ./build-stream-mix/flashvm_examples_multimedia_swf_stream_mix_main
```

No arguments prints usage without creating files; unrecognized extra arguments fail
before VM startup. `OUTPUT.pcm --queue` uses the composition adapter and a real
bounded memory FIFO. Each new block is deliberately rejected once before copying;
the original base is retried, yielding `Queue retained 5 rejected blocks.`. Only
after acceptance does a separate file consumer write the complete FIFO. Both
modes produce identical bytes; this FIFO has no real-time device consumption. An output path **replaces that file**, reporting `Mixed 16 frames after
source VM teardown.` after successful fclose. PCM is native-endian S16, with no WAV
header; interpret it as two channels at 48000 Hz. The independent Python oracle
uses native-endian packing and exact integer rational positions for every sample.
The same C/C++ ABI and example can be cross-built using the linked
[Linux](../../../doc/CROSS_LINUX_ZIG.md) or [Windows](../../../doc/CROSS_WINDOWS_ZIG.md)
toolchains; a cross-link does not imply execution on another CPU/OS.

## Ownership, errors and retry / Владение, ошибки и повтор

Preparation errors take the same centralized cleanup path before file creation.
Once fopen succeeds, later I/O errors are terminal and can leave partial output;
the example closes the file and native owners. It does not retry fwrite or undo
the old file replacement. Source snapshot outlives its converter; mixer owns its
voices; both close independently of the already destroyed VM. There is no live
managed root to pin inside the native loop. All calls are serial.

The direct file path reports errors terminally. The `--queue` path prepares its
adapter before source VM teardown. It retains both the original base witness and
already mixed bytes through rejection. Retrying identical original input sends
the retained mixture without another mixer_add. It closes the adapter before its
borrowed mixer. All calls remain serial, with no allocation during native mixing
or retry. Successful clear would discard pending PCM without rewinding voices;
this finite export never seeks and deliberately controls tail length itself.

В режиме `--queue` FIFO действительно копирует принятые данные, а каждый новый
блок сначала отклоняет без побочного эффекта. Пять отказов дают те же 64 байта,
что прямой экспорт. Fwrite исполняется отдельно после принятия всей очередью:
частичная запись файла не изображает атомарный отказ backend. Адаптер заимствует
mixer; закрывается раньше него. Общая desktop-политика timeline/seek не добавлена.

## Demonstrated C API / Использованный C API

| API | Purpose and contract / Назначение и контракт |
| --- | --- |
| [fvm_config_init](../../../include/flashvm/flashvm.h#L62) | Initialize the source runtime configuration before acquisition. |
| [fvm_runtime_create](../../../include/flashvm/flashvm.h#L64) | Acquire source runtime; release after independent preparation. |
| [fvm_context_create](../../../include/flashvm/flashvm.h#L68) | Own canonical loaded SWF on the preparation thread. |
| [fvm_load_memory](../../../include/flashvm/flashvm.h#L72) | Copy and validate the literal SWF before snapshots. |
| [fvm_swf_stream_sound_snapshot_open_pcm_bounded](../../../include/flashvm/swf_stream_sound.h#L96) | Clear output first; prepare supported PCM/ADPCM under explicit peak ceiling, return allocation/source statuses without leaking ownership. |
| [fvm_swf_sound_mixer_create_configured](../../../include/flashvm/swf_sound_mixer.h#L39) | Copy output geometry and reserve two native voice slots; failure publishes no mixer. |
| [fvm_swf_sound_mixer_submit](../../../include/flashvm/swf_sound_mixer.h#L50) | Owner snapshots each explicit event; allocation/control errors preserve existing voices. |
| [fvm_swf_stream_converter_init](../../../include/flashvm/swf_stream_convert.h#L47) | Borrow prepared source without allocation, with explicit rate/channels. |
| [fvm_context_destroy](../../../include/flashvm/flashvm.h#L70) | Release source canonical/managed ownership before native export. |
| [fvm_runtime_destroy](../../../include/flashvm/flashvm.h#L66) | End source VM lifetime; native snapshots remain retained. |
| [fvm_swf_stream_converter_next](../../../include/flashvm/swf_stream_convert.h#L58) | Return bounded initialized values or empty markers; EOF/errors preserve state/output; no VM/device/callback. |
| [fvm_swf_sound_mixer_add](../../../include/flashvm/swf_sound_mixer.h#L67) | Validate capacity, sum initialized base and event voices in place once, report scalar count; no allocation/callback/VM. Caller retains bytes for any later retry. |
| [`fvm_swf_sound_mix_queue_create`](../../../include/flashvm/swf_sound_mix_queue.h#L30) | [MIX_QUEUE.md](../../../doc/MIX_QUEUE.md): allocate fixed base/mixed buffers on owner; copy matching backend, borrow mixer exclusively; no device callback or VM; clear output on failure. |
| [`fvm_swf_sound_mix_queue_info`](../../../include/flashvm/swf_sound_mix_queue.h#L36) | [MIX_QUEUE.md](../../../doc/MIX_QUEUE.md): inspect retained frame count without polling device or allocating; wrong size/reentry preserves output. |
| [`fvm_swf_sound_mix_queue_close`](../../../include/flashvm/swf_sound_mix_queue.h#L40) | [MIX_QUEUE.md](../../../doc/MIX_QUEUE.md): after serial callback use, free adapter before borrowed mixer; no device close, voice stop or rewind. |
| [fvm_swf_sound_mixer_close](../../../include/flashvm/swf_sound_mixer.h#L74) | Release native voices after all serial reads finish; null allowed. |
| [fvm_swf_stream_sound_snapshot_close](../../../include/flashvm/swf_stream_sound.h#L131) | Release stream storage after converter use; null allowed. |

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
| `fvm_swf_sound_mix_queue_close` | function | [include/flashvm/swf_sound_mix_queue.h:40](../../../include/flashvm/swf_sound_mix_queue.h#L40) |
| `fvm_swf_sound_mix_queue_create` | function | [include/flashvm/swf_sound_mix_queue.h:30](../../../include/flashvm/swf_sound_mix_queue.h#L30) |
| `fvm_swf_sound_mix_queue_info` | function | [include/flashvm/swf_sound_mix_queue.h:36](../../../include/flashvm/swf_sound_mix_queue.h#L36) |
| `fvm_swf_sound_mixer_add` | function | [include/flashvm/swf_sound_mixer.h:67](../../../include/flashvm/swf_sound_mixer.h#L67) |
| `fvm_swf_sound_mixer_close` | function | [include/flashvm/swf_sound_mixer.h:74](../../../include/flashvm/swf_sound_mixer.h#L74) |
| `fvm_swf_sound_mixer_create_configured` | function | [include/flashvm/swf_sound_mixer.h:39](../../../include/flashvm/swf_sound_mixer.h#L39) |
| `fvm_swf_sound_mixer_submit` | function | [include/flashvm/swf_sound_mixer.h:50](../../../include/flashvm/swf_sound_mixer.h#L50) |
| `fvm_swf_stream_converter_init` | function | [include/flashvm/swf_stream_convert.h:47](../../../include/flashvm/swf_stream_convert.h#L47) |
| `fvm_swf_stream_converter_next` | function | [include/flashvm/swf_stream_convert.h:58](../../../include/flashvm/swf_stream_convert.h#L58) |
| `fvm_swf_stream_sound_snapshot_close` | function | [include/flashvm/swf_stream_sound.h:131](../../../include/flashvm/swf_stream_sound.h#L131) |
| `fvm_swf_stream_sound_snapshot_open_pcm_bounded` | function | [include/flashvm/swf_stream_sound.h:96](../../../include/flashvm/swf_stream_sound.h#L96) |
<!-- source-links:end -->
