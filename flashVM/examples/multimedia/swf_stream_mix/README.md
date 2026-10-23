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
| [fvm_swf_stream_sound_snapshot_open_pcm_bounded](../../../include/flashvm/swf_stream_sound.h#L102) | Clear output first; prepare supported PCM/ADPCM under explicit peak ceiling, return allocation/source statuses without leaking ownership. |
| [fvm_swf_sound_mixer_create_configured](../../../include/flashvm/swf_sound_mixer.h#L58) | Copy output geometry and reserve two native voice slots; failure publishes no mixer. |
| [fvm_swf_sound_mixer_submit](../../../include/flashvm/swf_sound_mixer.h#L76) | Owner snapshots each explicit event; allocation/control errors preserve existing voices. |
| [fvm_swf_stream_converter_init](../../../include/flashvm/swf_stream_convert.h#L48) | Borrow prepared source without allocation, with explicit rate/channels. |
| [fvm_context_destroy](../../../include/flashvm/flashvm.h#L70) | Release source canonical/managed ownership before native export. |
| [fvm_runtime_destroy](../../../include/flashvm/flashvm.h#L66) | End source VM lifetime; native snapshots remain retained. |
| [fvm_swf_stream_converter_next](../../../include/flashvm/swf_stream_convert.h#L67) | Return bounded initialized values or empty markers; EOF/errors preserve state/output; no VM/device/callback. |
| [fvm_swf_sound_mixer_add](../../../include/flashvm/swf_sound_mixer.h#L136) | Validate capacity, sum initialized base and event voices in place once, report scalar count; no allocation/callback/VM. Caller retains bytes for any later retry. |
| [`fvm_swf_sound_mix_queue_pump_tail`](../../../include/flashvm/swf_sound_mix_queue.h#L245) | Authorize one event-only zero-base block after producer EOF/quiescence; fixed frames/target, accepted cleared on failure. Retains mixed bytes on queue rejection, no allocation/VM/GC; host serializes adapter/mixer and owns both until quiescence. [EN](../../../doc/AUDIO_MIX_TAILS.md) / [RU](../../../doc/AUDIO_MIX_TAILS.ru.md); [declaration](../../../include/flashvm/swf_sound_mix_queue.h). |
| [`fvm_swf_sound_mix_queue_storage_required`](../../../include/flashvm/swf_sound_mix_queue.h#L44) | Predict exact owner plus buffer requests from channels/block, preserve scalar out on invalid/overflow; no allocation/callback/VM or borrowed owner acquisition. [EN](../../../doc/AUDIO_MIX_TAILS.md) / [RU](../../../doc/AUDIO_MIX_TAILS.ru.md); [declaration](../../../include/flashvm/swf_sound_mix_queue.h). |
| [`fvm_swf_sound_mix_queue_storage_info`](../../../include/flashvm/swf_sound_mix_queue.h#L96) | Copy exact-size fixed/buffer/total request observations; preserve out on invalid/reentry, no allocation/callback/VM/GC. Adapter remains host-owned and serialized. [EN](../../../doc/AUDIO_MIX_TAILS.md) / [RU](../../../doc/AUDIO_MIX_TAILS.ru.md); [declaration](../../../include/flashvm/swf_sound_mix_queue.h). |
| [`fvm_swf_sound_mix_queue_clock_begin`](../../../include/flashvm/swf_sound_mix_queue.h#L116) | Begin/restart explicit shared frame accounting only with no pending retry and an empty backend; no pause/clear/voice change, allocation/VM/GC. Borrow/serialize all owners; backend failures preserve the old epoch. [EN](../../../doc/MIX_QUEUE_CLOCK.md) / [RU](../../../doc/MIX_QUEUE_CLOCK.ru.md); [declaration](../../../include/flashvm/swf_sound_mix_queue.h). |
| [`fvm_swf_sound_mix_queue_clock_info`](../../../include/flashvm/swf_sound_mix_queue.h#L133) | Copy exact-size fresh submitted/queued/played/seconds values for accepted base and tails; caller owns snapshot. Invalid/backend/regression errors preserve out; no allocation/VM/GC and no DAC timestamp. [EN](../../../doc/MIX_QUEUE_CLOCK.md) / [RU](../../../doc/MIX_QUEUE_CLOCK.ru.md); [declaration](../../../include/flashvm/swf_sound_mix_queue.h). |
| [`fvm_swf_sound_mix_queue_clock_end`](../../../include/flashvm/swf_sound_mix_queue.h#L125) | After quiescing producers and draining FIFO, disable shared accounting. Pending/busy rejects, nonempty/inactive returns NOT_FOUND; no implicit ownership transfer, pause/clear, allocation/VM/GC. [EN](../../../doc/MIX_QUEUE_CLOCK.md) / [RU](../../../doc/MIX_QUEUE_CLOCK.ru.md); [declaration](../../../include/flashvm/swf_sound_mix_queue.h). |
| [`fvm_swf_sound_mix_queue_set_identity_tracking`](../../../include/flashvm/swf_sound_mix_queue.h#L148) | Preallocate bounded optional ID deadlines in a zero epoch, or free them after observed drain and voice retirement. No callbacks/VM/GC; old mode survives allocation failure; all controls must use the tracked owner. [Guide](../../../doc/MIX_QUEUE_IDENTITIES.md); [declaration](../../../include/flashvm/swf_sound_mix_queue.h). |
| [`fvm_swf_sound_mix_queue_submit_prepared_bounded`](../../../include/flashvm/swf_sound_mix_queue.h#L162) | Adopt a caller-owned native voice after accepted/pending frames and register its deadline. Success clears its handle; failure preserves ownership/progress. Queued NoMultiple consumes unused starts; mixer budget excludes adapter/table. No allocation/VM/GC; backend observation can block. [Guide](../../../doc/MIX_QUEUE_IDENTITIES.md); [declaration](../../../include/flashvm/swf_sound_mix_queue.h). |
| [`fvm_swf_sound_mix_queue_identity_info`](../../../include/flashvm/swf_sound_mix_queue.h#L180) | Copy exact-size ID/dequeue/end/until/active diagnostics using a fresh FIFO observation; errors preserve out. No allocation/VM/GC; caller owns snapshot, not native state. [Guide](../../../doc/MIX_QUEUE_IDENTITIES.md); [declaration](../../../include/flashvm/swf_sound_mix_queue.h). |
| [`fvm_swf_sound_voice_open_bounded`](../../../include/flashvm/swf_sound.h#L97) | Decode/copy one event under a separate voice-snapshot ceiling on source owner; return an independent native handle. Allocation/codec errors preserve null out. Adopt or close before losing ownership; no device/thread acquired. [Guide](../../../doc/MIX_QUEUE_IDENTITIES.md); [declaration](../../../include/flashvm/swf_sound.h). |
| [`fvm_swf_sound_voice_close`](../../../include/flashvm/swf_sound.h#L128) | Release a still caller-owned prepared voice; null after successful adoption is safe. Quiesce readers first; do not close a consumed mixer-owned handle. No device/VM callback. [Guide](../../../doc/MIX_QUEUE_IDENTITIES.md); [declaration](../../../include/flashvm/swf_sound.h). |
| [`fvm_swf_sound_mix_queue_create_bounded`](../../../include/flashvm/swf_sound_mix_queue.h#L226) | Validate geometry then acquire fixed adapter storage within the exact byte ceiling; clear native out, preserve adapted on failure. Borrow mixer/backend; no callbacks/VM, quiesce before close. [EN](../../../doc/AUDIO_MIX_TAILS.md) / [RU](../../../doc/AUDIO_MIX_TAILS.ru.md); [declaration](../../../include/flashvm/swf_sound_mix_queue.h). |
| [`fvm_swf_sound_mix_queue_pending`](../../../include/flashvm/swf_sound_mix_queue.h#L31) | Copy exact-size NONE/BASE/TAIL retry ownership without callbacks, allocation or voice advance; preserve out on invalid/reentry. Observation is not a reservation; host serializes all owners. [EN](../../../doc/AUDIO_MIX_TAILS.md) / [RU](../../../doc/AUDIO_MIX_TAILS.ru.md); [declaration](../../../include/flashvm/swf_sound_mix_queue.h). |
| [`fvm_swf_sound_mix_queue_create`](../../../include/flashvm/swf_sound_mix_queue.h#L214) | [MIX_QUEUE.md](../../../doc/MIX_QUEUE.md): allocate fixed base/mixed buffers on owner; copy matching backend, borrow mixer exclusively; no device callback or VM; clear output on failure. |
| [`fvm_swf_sound_mix_queue_info`](../../../include/flashvm/swf_sound_mix_queue.h#L274) | [MIX_QUEUE.md](../../../doc/MIX_QUEUE.md): inspect retained frame count without polling device or allocating; wrong size/reentry preserves output. |
| [`fvm_swf_sound_mix_queue_close`](../../../include/flashvm/swf_sound_mix_queue.h#L278) | [MIX_QUEUE.md](../../../doc/MIX_QUEUE.md): after serial callback use, free adapter before borrowed mixer; no device close, voice stop or rewind. |
| [fvm_swf_sound_mixer_close](../../../include/flashvm/swf_sound_mixer.h#L143) | Release native voices after all serial reads finish; null allowed. |
| [fvm_swf_stream_sound_snapshot_close](../../../include/flashvm/swf_stream_sound.h#L160) | Release stream storage after converter use; null allowed. |

## Native tail pump / Подача хвостов

`OUTPUT.pcm --tails` uses the retained adapter for stream input and
[fvm_swf_sound_mix_queue_pump_tail](../../../include/flashvm/swf_sound_mix_queue.h#L245) only after actual converter EOF. It keeps the
explicit 16-frame export budget, retries each rejection and verifies that exhausted
voices do not enqueue further silence. The resulting 64 bytes equal both existing
modes under the independent Python oracle. No clock, frontend or physical device
is introduced. Режим использует настоящий tail API после EOF, сохраняет конечный
бюджет и проверяет точный повтор и исчерпание. [EN contract](../../../doc/AUDIO_MIX_TAILS.md) /
[RU](../../../doc/AUDIO_MIX_TAILS.ru.md).

`OUTPUT.pcm --bounded-tails` additionally predicts and enforces the exact adapter-only
storage ceiling, observes actual fixed requests and checks BASE/TAIL pending origin.
All four modes produce identical PCM under the independent oracle. It reports
32 bytes of PCM arrays plus platform-dependent fixed owner bytes; allocator,
software FIFO, mixer/voices and VM are excluded. Режим явно разделяет бюджет
адаптера и заимствованных источников; наблюдение pending не резервирует очередь.

## Shared FIFO clock / Общий счётчик FIFO

`OUTPUT.pcm --clock` uses bounded acquisition and explicit tail pumping, and begins
shared accounting before native production. Before software consumption it prints
`Shared clock submitted=16 queued=16 played=0`; after the complete file write,
the software consumer acknowledges drain and it prints
`Shared clock submitted=16 queued=0 played=16 seconds=0.000333333`, then ends the
epoch. Exact PCM equals all existing modes. This is software FIFO dequeue,
not a DAC timestamp or real device/A/V synchronization. File flush errors remain
terminal and do not pretend to restore consumed voice time.
Режим явно начинает учёт, наблюдает base+tail, подтверждает drain только после
полного fwrite и завершает отсчёт. [EN](../../../doc/MIX_QUEUE_CLOCK.md) /
[RU](../../../doc/MIX_QUEUE_CLOCK.ru.md).

## Queued identities / Очередные ID

`OUTPUT.pcm --identities` prepares independent native voices, enables shared clock
and an exactly budgeted ID table, then adopts every event through tracked APIs.
The longer event prints `Identity id=8 end=14 played=0 active=1` even after mixer
retirement, then `Identity id=8 end=14 played=16 active=0` after software drain.
It cancels the expired ID, safely disables tracking and ends the clock. Exact PCM
equals all five earlier modes. Each temporary voice and aggregate adopted mixer
has its own explicit ceiling; these exclude adapter/table/software FIFO/VM memory.
NoMultiple suppression is tested by native fixtures; this embedded example uses
ordinary starts. Режим проверяет queued-ID до/после drain, не физическую слышимость.
[EN contract](../../../doc/MIX_QUEUE_IDENTITIES.md) / [RU](../../../doc/MIX_QUEUE_IDENTITIES.ru.md).


The `--identities` path predicts the full adapter ceiling before enabling deadlines,
then uses event-only global cancellation after the exported FIFO drains. This control
retains accepted PCM and does not stop or rewind a base producer. It is immediate,
including existing DECLICK voices; device-wide cancellation still requires acknowledged clear.

| API | Contract and ownership |
| --- | --- |
| [`fvm_swf_sound_mix_queue_storage_required_for_tracking`](../../../include/flashvm/swf_sound_mix_queue.h#L52) | Predict owner/PCM/optional ID bytes into a required scalar without allocation, callbacks or VM access; invalid geometry/mode preserves output. Excludes mixer, voices and device. [Guide](../../../doc/MIX_QUEUE_IDENTITIES.md). |
| [`fvm_swf_sound_mix_queue_stop_all`](../../../include/flashvm/swf_sound_mix_queue.h#L201) | Retire all future events and reset ID deadlines on a live tracked owner; preserve pending/FIFO bytes and clock. Requires external serialization; no callback/allocation/VM/GC. Disabled tracking is UNSUPPORTED. [Guide](../../../doc/MIX_QUEUE_IDENTITIES.md). |


Desktop player integration now uses the same native mixer/ID ownership with a paired stream-output tail operation. This executable remains the six-mode memory-FIFO exporter; it does not run the desktop worker. [Production integration and remaining limits](../../../doc/DESKTOP_MIX.md).


## Gated C89 export / Экспорт с удержанием base

`OUTPUT.pcm --gated` adds a seventh executable mode: two four-frame event-only
blocks hold the source converter unchanged, then frame one resumes its nine base
frames. Exactly 17 stereo frames (68 bytes) are exported after source VM teardown.
Every actual write is rejected once; retry opens the gate immediately, proving
retained TAIL ownership takes priority over newly authorized BASE. This is an
explicit memory-FIFO policy, not a device timer or automatic movie synchronization.
[Gate contract EN](../../../doc/MIXED_GATES.md) / [RU](../../../doc/MIXED_GATES.ru.md).

Режим `--gated`: восемь event-only кадров без продвижения stream, затем девять
base-кадров. Всего 17 stereo кадров; каждый failed write повторяется без remix,
даже если между отказом и retry разрешить base. Файловый consumer отдельный.

| API | Purpose, ownership and restrictions |
| --- | --- |
| [`fvm_swf_stream_output_init`](../../../include/flashvm/swf_stream_output.h#L45) | Borrow converter, PCM retry buffer and the exact adapted backend into caller-owned producer; empty FIFO, matching geometry, no allocation/VM. All dependencies outlive serial use. [Contract](../../../doc/STREAM_PCM_OUTPUT.md). |
| [`fvm_swf_stream_output_pump_mix_until`](../../../include/flashvm/swf_stream_output.h#L104) | Select authorized BASE or event-only gate work and accept retained TAIL before changed gate input; optional accepted cleared first. No allocation/VM/GC, external serialization, keep both owners and retry bytes live on failure. [Contract](../../../doc/MIXED_GATES.md). |

## Partial headroom / Неполный остаток FIFO

Run `OUTPUT.pcm --headroom` for a 14-frame (56-byte) event-only export. The base
converter stays unchanged after VM teardown. Each call admits one frame by raising
the FIFO target by one; every short block is rejected once. Retrying with the larger
queue target preserves the original one-frame size. Expected: fourteen rejected
blocks, submitted/played14 after file consumption, ID8 end14 and inactive. This is
a deterministic software headroom demonstration, not a timed device consumer.

`--headroom` экспортирует 14 event-only кадров без продвижения base и без padding.
Каждый короткий блок отклоняется; повтор с большим target сохраняет исходный размер.
FIFO потребляется записью файла после всех принятых блоков; это не реальное устройство.

| API | Purpose, ownership and contract |
| --- | --- |
| [`fvm_swf_sound_mix_queue_pump_tail_up_to`](../../../include/flashvm/swf_sound_mix_queue.h#L261) | Synchronously observe the borrowed FIFO and mix at most the supplied maximum into available room. Optional accepted clears first. Retained TAIL is indivisible; caller quiesces base and serializes all owners. No allocation/VM/GC; callbacks may block. [Full statuses and lifetime](../../../doc/ADAPTIVE_TAILS.md). |

## Finite event tails / Конечные событийные хвосты

`OUTPUT.pcm --finite` exports the same 14 event-only frames (56 bytes) as headroom
mode using four bounded blocks of 4,4,4,2 frames. Each block is rejected once;
retry must preserve its pre-render live horizon. The base converter stays unchanged.
After software drain, submitted/played14 and ID8 end14/inactive are expected.
This ninth mode demonstrates native finite completion; desktop uses the explicit
`--finite-mixed-tails` option. Neither is physical DAC/reference A/V acceptance.

`--finite` пишет 14 event-only кадров блоками 4,4,4,2, без завершающего padding.
Четыре отклонённых блока повторяются точно; base не продвигается. После записи файла
played14 и ID8 inactive. Это конечный нативный вывод, не физическая A/V-приёмка.

| API | Purpose, ownership and contract |
| --- | --- |
| [`fvm_swf_sound_mixer_active_frames`](../../../include/flashvm/swf_sound_mixer.h#L54) | Copy the bounded current-grid horizon without reading PCM or consuming lookahead. Includes repeats/release, OK/zero when empty. Caller retains and serializes mixer; no allocation/callback/VM/GC. [Bounds/statuses](../../../doc/FINITE_TAILS.md). |
| [`fvm_swf_sound_mix_queue_pump_tail_finite`](../../../include/flashvm/swf_sound_mix_queue.h#L270) | Observe the exclusive FIFO and render no more than available room and the live horizon. Existing TAIL remains indivisible; optional accepted clears first. Quiesce base and serialize borrowed owners; callbacks may block, no allocation/VM/GC. [Ownership/statuses](../../../doc/FINITE_TAILS.md). |

## BASE/TAIL categories / Категории BASE/TAIL

The tenth mode `OUTPUT.pcm --origins` preserves the gated 17-frame rational PCM
oracle while accounting separately for nine BASE and eight TAIL frames. All five
blocks are deliberately rejected once. Before file consumption it prints
`Origins base=9/0 tail=8/0`; after acknowledged software drain, `base=9/9 tail=8/8`.
The bitmap is explicitly budgeted/enabled before voice admission and disabled after
FIFO/IDs drain before shared clock end. This is output-origin bookkeeping, not SWF
source phase, physical DAC time or automatic A/V policy.

Десятый режим --origins сохраняет 17 кадров gated PCM, отдельно считая BASE9 и
TAIL8. Пять отклонённых блоков повторяются без двойного счёта. До записи файла
played0; после подтверждённого drain — BASE9/9 и TAIL8/8. Bitmap выделяется по
явному бюджету; режим отключается после drain до clock end. Это категории вывода,
не SWF/DAC timestamps или автоматическая A/V-синхронизация.

| API | Purpose, ownership and contract |
| --- | --- |
| [`fvm_swf_sound_mix_queue_storage_required_for_modes`](../../../include/flashvm/swf_sound_mix_queue.h#L60) | Predict exact owner/PCM/ID/origin allocation requests without allocation/callback/VM. Boolean modes, out preserved on error; excludes other owners. [Storage contract](../../../doc/MIX_ORIGIN_CLOCK.md). |
| [`fvm_swf_sound_mix_queue_set_origin_tracking`](../../../include/flashvm/swf_sound_mix_queue.h#L84) | Allocate the fixed accepted-origin bitmap at zero shared epoch or free it after observed drain/no retry. Caller retains and serializes queue/mixer/backend; callbacks may block, allocation is explicit. [Mode lifetime](../../../doc/MIX_ORIGIN_CLOCK.md). |
| [`fvm_swf_sound_mix_queue_origin_info`](../../../include/flashvm/swf_sound_mix_queue.h#L91) | Fresh BASE/TAIL accepted/dequeued/queued snapshot, excluding pending retry. Errors preserve exact-size out. No allocation/render/VM/GC; borrowed owners stay serialized/live. [Category clock](../../../doc/MIX_ORIGIN_CLOCK.md). |

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
| `fvm_swf_sound_mix_queue_clock_begin` | function | [include/flashvm/swf_sound_mix_queue.h:116](../../../include/flashvm/swf_sound_mix_queue.h#L116) |
| `fvm_swf_sound_mix_queue_clock_end` | function | [include/flashvm/swf_sound_mix_queue.h:125](../../../include/flashvm/swf_sound_mix_queue.h#L125) |
| `fvm_swf_sound_mix_queue_clock_info` | function | [include/flashvm/swf_sound_mix_queue.h:133](../../../include/flashvm/swf_sound_mix_queue.h#L133) |
| `fvm_swf_sound_mix_queue_close` | function | [include/flashvm/swf_sound_mix_queue.h:278](../../../include/flashvm/swf_sound_mix_queue.h#L278) |
| `fvm_swf_sound_mix_queue_create` | function | [include/flashvm/swf_sound_mix_queue.h:214](../../../include/flashvm/swf_sound_mix_queue.h#L214) |
| `fvm_swf_sound_mix_queue_create_bounded` | function | [include/flashvm/swf_sound_mix_queue.h:226](../../../include/flashvm/swf_sound_mix_queue.h#L226) |
| `fvm_swf_sound_mix_queue_identity_info` | function | [include/flashvm/swf_sound_mix_queue.h:180](../../../include/flashvm/swf_sound_mix_queue.h#L180) |
| `fvm_swf_sound_mix_queue_info` | function | [include/flashvm/swf_sound_mix_queue.h:274](../../../include/flashvm/swf_sound_mix_queue.h#L274) |
| `fvm_swf_sound_mix_queue_origin_info` | function | [include/flashvm/swf_sound_mix_queue.h:91](../../../include/flashvm/swf_sound_mix_queue.h#L91) |
| `fvm_swf_sound_mix_queue_pending` | function | [include/flashvm/swf_sound_mix_queue.h:31](../../../include/flashvm/swf_sound_mix_queue.h#L31) |
| `fvm_swf_sound_mix_queue_pump_tail` | function | [include/flashvm/swf_sound_mix_queue.h:245](../../../include/flashvm/swf_sound_mix_queue.h#L245) |
| `fvm_swf_sound_mix_queue_pump_tail_finite` | function | [include/flashvm/swf_sound_mix_queue.h:270](../../../include/flashvm/swf_sound_mix_queue.h#L270) |
| `fvm_swf_sound_mix_queue_pump_tail_up_to` | function | [include/flashvm/swf_sound_mix_queue.h:261](../../../include/flashvm/swf_sound_mix_queue.h#L261) |
| `fvm_swf_sound_mix_queue_set_identity_tracking` | function | [include/flashvm/swf_sound_mix_queue.h:148](../../../include/flashvm/swf_sound_mix_queue.h#L148) |
| `fvm_swf_sound_mix_queue_set_origin_tracking` | function | [include/flashvm/swf_sound_mix_queue.h:84](../../../include/flashvm/swf_sound_mix_queue.h#L84) |
| `fvm_swf_sound_mix_queue_stop_all` | function | [include/flashvm/swf_sound_mix_queue.h:201](../../../include/flashvm/swf_sound_mix_queue.h#L201) |
| `fvm_swf_sound_mix_queue_storage_info` | function | [include/flashvm/swf_sound_mix_queue.h:96](../../../include/flashvm/swf_sound_mix_queue.h#L96) |
| `fvm_swf_sound_mix_queue_storage_required` | function | [include/flashvm/swf_sound_mix_queue.h:44](../../../include/flashvm/swf_sound_mix_queue.h#L44) |
| `fvm_swf_sound_mix_queue_storage_required_for_modes` | function | [include/flashvm/swf_sound_mix_queue.h:60](../../../include/flashvm/swf_sound_mix_queue.h#L60) |
| `fvm_swf_sound_mix_queue_storage_required_for_tracking` | function | [include/flashvm/swf_sound_mix_queue.h:52](../../../include/flashvm/swf_sound_mix_queue.h#L52) |
| `fvm_swf_sound_mix_queue_submit_prepared_bounded` | function | [include/flashvm/swf_sound_mix_queue.h:162](../../../include/flashvm/swf_sound_mix_queue.h#L162) |
| `fvm_swf_sound_mixer_active_frames` | function | [include/flashvm/swf_sound_mixer.h:54](../../../include/flashvm/swf_sound_mixer.h#L54) |
| `fvm_swf_sound_mixer_add` | function | [include/flashvm/swf_sound_mixer.h:136](../../../include/flashvm/swf_sound_mixer.h#L136) |
| `fvm_swf_sound_mixer_close` | function | [include/flashvm/swf_sound_mixer.h:143](../../../include/flashvm/swf_sound_mixer.h#L143) |
| `fvm_swf_sound_mixer_create_configured` | function | [include/flashvm/swf_sound_mixer.h:58](../../../include/flashvm/swf_sound_mixer.h#L58) |
| `fvm_swf_sound_mixer_submit` | function | [include/flashvm/swf_sound_mixer.h:76](../../../include/flashvm/swf_sound_mixer.h#L76) |
| `fvm_swf_sound_voice_close` | function | [include/flashvm/swf_sound.h:128](../../../include/flashvm/swf_sound.h#L128) |
| `fvm_swf_sound_voice_open_bounded` | function | [include/flashvm/swf_sound.h:97](../../../include/flashvm/swf_sound.h#L97) |
| `fvm_swf_stream_converter_init` | function | [include/flashvm/swf_stream_convert.h:48](../../../include/flashvm/swf_stream_convert.h#L48) |
| `fvm_swf_stream_converter_next` | function | [include/flashvm/swf_stream_convert.h:67](../../../include/flashvm/swf_stream_convert.h#L67) |
| `fvm_swf_stream_output_init` | function | [include/flashvm/swf_stream_output.h:45](../../../include/flashvm/swf_stream_output.h#L45) |
| `fvm_swf_stream_output_pump_mix_until` | function | [include/flashvm/swf_stream_output.h:104](../../../include/flashvm/swf_stream_output.h#L104) |
| `fvm_swf_stream_sound_snapshot_close` | function | [include/flashvm/swf_stream_sound.h:160](../../../include/flashvm/swf_stream_sound.h#L160) |
| `fvm_swf_stream_sound_snapshot_open_pcm_bounded` | function | [include/flashvm/swf_stream_sound.h:102](../../../include/flashvm/swf_stream_sound.h#L102) |
<!-- source-links:end -->
