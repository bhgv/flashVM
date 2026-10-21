# swf_stream — frame-addressed streaming PCM into a native file bridge

[Русский контракт](../../../doc/STREAM_PCM_READER.ru.md) / [English contract](../../../doc/STREAM_PCM_READER.md).

Sources: [main.c](main.c), [example_constants.h](example_constants.h).

## Назначение

Исполняемый C89 пример перечисляет корневой streaming-sound segment и его
SoundStreamBlock из встроенного FWS4. Исходный PCM — stereo 44100 Hz U8;
playback recommendation намеренно mono 11025 Hz. Пример использует source
geometry, показывает variable-length blocks и пропущенный movie frame.
Это явный экспорт, без real-time воспроизведения, устройства или скрытого clock.

Каждая непустая строка source/header подписана; ошибок через assert нет.
Host bridge вызывает настоящие fopen/fwrite/fclose, его можно заменить своим
системным выходом с тем же [FVM_MediaOutput](../../../include/flashvm/multimedia.h#L68) ABI. Мост синхронно заимствует PCM.

## Подготовка и запуск

Из корня проекта:

```sh
cmake -S . -B build-stream -DFVM_DEFS_FILE=cmake/profiles/minimal.defs -DCMAKE_BUILD_TYPE=Release
cmake --build build-stream --target flashvm_examples_multimedia_swf_stream_main --parallel
./build-stream/flashvm_examples_multimedia_swf_stream_main OUTPUT.pcm
```

Нужны C89/C++98 toolchain и writable output path. NO_MEDIA/NO_GRAPHICS достаточны:
FFmpeg, ALSA, модель распознавания, permissions и реальные audio devices не нужны.
Названный файл может быть перезаписан. Без аргументов только usage, без VM/файлов.
На Windows/macOS/Android замените путь/запуск согласно native toolchain; Linux
проверка не заявляется native acceptance этих ОС. Встроенный input не требует
загрузки внешнего SWF или установки Flash Player.

## Результат и реализация

```text
Frame 1: 2 sample frames
Frame 3: 1 sample frames
Frame 4: 0 sample frames
Exported 3 sample frames from 3 blocks.
```

Файл — 12 bytes native-endian S16 stereo:
`[-32768, 32512, 0, 0, 32512, -32768]`. Frame 2 отсутствует и не заполняется
тишиной. Среднее в заголовке равно 1, но первый block содержит 2 sample frames;
последний empty block учитывается в metadata и не вызывает output write.
Буфер содержит всего один stereo frame, первый блок читается двумя ranges.

[GC:] Context owns canonical SWF bytes и регистрацию output. Header — copied
scalars; block data — borrowed до успешного reload/context_destroy. Пример
никогда не хранит payload pointer для callback: read_pcm сначала копирует в
caller short buffer, затем channel_write синхронно вызывает FILE bridge.
В callbacks этого примера нет reload/GC; если вы их добавите, заново проверяйте
ordinals/geometry после возврата — старый block view может стать невалидным.

FILE/userdata принадлежат host и живут до конца context_destroy. Stop-all
закрывает actual output; terminal cleanup повторяет остановку ещё active канала.
При failed write FILE может остаться вне active state: fallback fclose после
context teardown освобождает такой handle. Runtime освобождается последним.
Ошибки не вызывают неявного повтора уже доставленных bytes.

Для своего системного моста замените write_pcm/stop_pcm; соблюдайте синхронное
заимствование, owning VM thread, независимое владение устройством и запрет
GC/registry mutation внутри callback. См. [MULTIMEDIA_BRIDGES.md](../../../doc/MULTIMEDIA_BRIDGES.md)
и [ALSA_AUDIO_OUTPUT.md](../../../doc/ALSA_AUDIO_OUTPUT.md). Host сам реализует
реальное время, gap silence, rate/channel conversion, смену segment и seek.
Этот пример не декодирует compressed stream и не исполняет MovieClip timeline.

## Демонстрируемые API

- [`fvm_swf_stream_sound_count`](../../../include/flashvm/swf_stream_sound.h#L53), [`fvm_swf_stream_sound_at`](../../../include/flashvm/swf_stream_sound.h#L55) — сегменты и copied header:
  [SWF_STREAM_SOUND.md](../../../doc/SWF_STREAM_SOUND.md#fvm_swf_stream_sound_count),
  [header query](../../../doc/SWF_STREAM_SOUND.md#fvm_swf_stream_sound_at).
- [`fvm_swf_stream_sound_block_count`](../../../include/flashvm/swf_stream_sound.h#L57), [`fvm_swf_stream_sound_block_at`](../../../include/flashvm/swf_stream_sound.h#L59) — actual frame,
  empty/missing blocks и borrowed payload:
  [block count](../../../doc/SWF_STREAM_SOUND.md#fvm_swf_stream_sound_block_count),
  [block view](../../../doc/SWF_STREAM_SOUND.md#fvm_swf_stream_sound_block_at).
- [`fvm_swf_stream_sound_read_pcm`](../../../include/flashvm/swf_stream_sound.h#L61) — bounded copies без callbacks/allocations:
  [range contract](../../../doc/SWF_STREAM_SOUND.md#fvm_swf_stream_sound_read_pcm).
- [`fvm_audio_channel_create`](../../../include/flashvm/multimedia.h#L82), [`fvm_audio_channel_write`](../../../include/flashvm/multimedia.h#L84), [`fvm_audio_stop_all`](../../../include/flashvm/multimedia.h#L92) —
  copied output table, actual native writes, active state and stop:
  [AUDIO_CHANNELS.md](../../../doc/AUDIO_CHANNELS.md).
- [`fvm_config_init`](../../../include/flashvm/flashvm.h#L62), [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L64), [`fvm_context_create`](../../../include/flashvm/flashvm.h#L68), [`fvm_load_memory`](../../../include/flashvm/flashvm.h#L72),
  [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L70), [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L66) — dependency-ordered VM ownership:
  [API_INDEX.md](../../../doc/API_INDEX.md#runtime-and-context),
  [canonical input](../../../doc/SWF_SOUND_RESOURCES.md#scope--область-реализации).

## English

This line-commented C89 tutorial reads a literal FWS4 stream with unequal stereo
PCM blocks, a missing movie frame and an explicit empty block. Advisory playback
geometry differs from source geometry; only source rate/channels drive output.
It reads one sample frame at a time into caller shorts, then submits through
real synchronous stdio callbacks. Output is exactly three stereo frames (12 bytes)
with the values shown above. No timing/silence insertion or audible playback is
implied. NO_MEDIA/NO_GRAPHICS and an explicit writable output path suffice.

The context owns canonical bytes and channel registration; the host owns FILE,
userdata and caller buffers. Borrowed block data must not survive reload; decoded
shorts do. Stop-all flushes/closes actual output, context cleanup runs while
userdata is live, and fallback fclose handles failed-write ownership before
runtime destruction. No arguments create no files/VM; native errors use normal
cleanup without assertions or implicit replay. Native device adapters, timing,
compressed codecs and foreign-platform acceptance are separate concerns.

## Validation / Проверка исполняемого примера

```sh
python3 scripts/check_swf_stream_example.py build-stream/flashvm_examples_multimedia_swf_stream_main
python3 scripts/check_swf_stream_example.py build-stream/flashvm_examples_multimedia_swf_stream_main --ffmpeg
```

Первая проверка запускает настоящий бинарник, сверяет frame metadata и все bytes,
проверяет замену файла, отсутствие side effects без аргументов и cleanup при
ошибке native path. Она включена в native CTest как
`flashvm_swf_stream_tutorial_host`; cross builds не запускают целевой binary на
host. Второй вариант дополнительно требует системный ffmpeg и сверяет raw U8
→ native S16 stereo conversion. Это независимый PCM oracle, а не доказательство
полного SWF demux/stream sync в FFmpeg и не runtime dependency FlashVM.

The native CTest harness exercises the real executable, exact metadata/PCM,
replacement and failure cleanup. Optional --ffmpeg checks independently known
raw payload conversion; it is neither a runtime dependency nor a complete SWF
playback oracle. Cross builds omit this host-side process harness.

## Prepared mode / Подготовленный режим

Pass `--prepared` after the output path:

```sh
./build-jit/flashvm_examples_multimedia_swf_stream_main output.pcm --prepared
```

This mode explicitly prepares an immutable indexed PCM snapshot, destroys the
original context and runtime, then creates a new **unloaded** VM only for the
ordinary file-output registration. Header queries and bounded PCM reads therefore
come entirely from the independent snapshot. Output and movie-frame diagnostics
are identical to the default live mode. Preparation can fail with OUT_OF_MEMORY;
cleanup releases the snapshot independently of output-VM ownership. No additional
system package is required. It trades one payload/index copy for constant-time
block lookup instead of repeatedly scanning all root tags.

Режим явно копирует PCM и индекс, уничтожает исходную VM и создаёт новую,
**без загруженного SWF**, только для файлового переходника. Метаданные и PCM
берутся из независимого снимка; вывод полностью совпадает с обычным режимом.
Подготовка требует памяти, очистка снимка независима от очистки новой VM.

Additional demonstrated API (contracts, limits and lifetime in
[SWF_STREAM_SOUND.md](../../../doc/SWF_STREAM_SOUND.md#prepared-immutable-pcm-index--подготовленный-неизменяемый-индекс-pcm)):

- [`fvm_swf_stream_sound_snapshot_open`](../../../include/flashvm/swf_stream_sound.h#L65): validate, budget and copy the selected segment.
- [`fvm_swf_stream_sound_snapshot_info`](../../../include/flashvm/swf_stream_sound.h#L127): inspect the copied source/playback header.
- [`fvm_swf_stream_sound_snapshot_block_count`](../../../include/flashvm/swf_stream_sound.h#L129): enumerate the prepared index.
- [`fvm_swf_stream_sound_snapshot_block_at`](../../../include/flashvm/swf_stream_sound.h#L131): borrow original-wire bytes until close.
- [`fvm_swf_stream_sound_snapshot_read_pcm`](../../../include/flashvm/swf_stream_sound.h#L135): convert bounded independent ranges.
- [`fvm_swf_stream_sound_snapshot_close`](../../../include/flashvm/swf_stream_sound.h#L160): release native storage even after VM teardown.

The host checker runs both modes, compares exact stdout/native-S16 bytes and checks
actual missing-directory write failure cleanup in both modes. This is file bridge
acceptance, not physical audio-device or automatic MovieClip clock acceptance.

## Direct native cursor / Прямой native курсор

```sh
# From the repository root / Из корня репозитория:
cmake -S flashVM -B build-stream -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/minimal.defs"
cmake --build build-stream --target flashvm_examples_multimedia_swf_stream_main
./build-stream/flashvm_examples_multimedia_swf_stream_main OUTPUT.pcm --cursor
python3 flashVM/scripts/check_swf_stream_example.py ./build-stream/flashvm_examples_multimedia_swf_stream_main
```

`--cursor` prepares copied encoded storage and destroys the source runtime/context
before any file output. Direct C functions then read one stereo pair at a time,
preserve frame/block boundaries and write native S16. No output VM is recreated.
The caller owns the FILE and snapshot until export completes; cleanup closes
both even on errors. Reader has no destructor or allocation. This is explicit
export, not clocked audio playback. Expected report and exact PCM remain identical
to default/`--prepared`; native byte order is intentional for this raw file.

`--cursor` готовит копию encoded source и уничтожает VM до вывода файла. Прямые
C функции читают по стереопаре с границами кадров/блоков и пишут native S16;
новая output VM не создаётся. Caller владеет FILE/snapshot до завершения,
cleanup закрывает их и при ошибке. Reader не выделяет память и не требует
деструктора. Это экспорт, не синхронизированное воспроизведение. Отчёт и PCM
совпадают с default/`--prepared`; raw файл намеренно имеет native byte order.

| API | Purpose, ownership and execution / Назначение и владение |
| --- | --- |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h#L62) | Initialize caller config synchronously before ownership acquisition. Подготавливает descriptor без запуска VM. |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L64) | Acquire runtime; caller destroys after contexts. Получает runtime до context, ошибки идут в cleanup. |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h#L68) | Acquire source or older prepared-mode output context; runtime borrowed. Создаёт context с явно живым runtime. |
| [`fvm_load_memory`](../../../include/flashvm/flashvm.h#L72) | Copy/validate embedded SWF on VM owner before preparing native storage. Копирует SWF, источник массива статический. |
| [`fvm_swf_stream_sound_count`](../../../include/flashvm/swf_stream_sound.h#L53) | Validate root stream controls and copy count before output. Проверяет весь поток до side effects. |
| [`fvm_swf_stream_sound_at`](../../../include/flashvm/swf_stream_sound.h#L55) | Copy actual source/advisory header; no payload ownership transfer. Копирует геометрию. |
| [`fvm_swf_stream_sound_block_count`](../../../include/flashvm/swf_stream_sound.h#L57) | Copy encoded block count, not movie-frame count. Различает блоки и кадры. |
| [`fvm_swf_stream_sound_snapshot_open`](../../../include/flashvm/swf_stream_sound.h#L65) | Own independent encoded bytes/index; allocation may fail before output. Создаёт native копию с отдельным владением. |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L70) | Release source before cursor output, or context callbacks before userdata cleanup. Уничтожает context до runtime. |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L66) | Release only after its contexts; copied snapshot survives. Снимок не заимствует runtime. |
| [`fvm_swf_stream_sound_snapshot_info`](../../../include/flashvm/swf_stream_sound.h#L127) | Copy source geometry after original VM teardown. Без VM и allocation. |
| [`fvm_swf_stream_sound_snapshot_block_count`](../../../include/flashvm/swf_stream_sound.h#L129) | Query immutable indexed count after teardown. Заимствует snapshot лишь на вызов. |
| [`fvm_swf_stream_pcm_reader_init`](../../../include/flashvm/swf_stream_pcm.h#L40) | Borrow immutable snapshot, initialize native cursor without allocation. Caller keeps snapshot alive. [Contract](../../../doc/STREAM_PCM_READER.md). |
| [`fvm_swf_stream_pcm_reader_next`](../../../include/flashvm/swf_stream_pcm.h#L48) | Copy bounded S16 and owned metadata; EOF/errors preserve outputs. Single owner, no VM/I/O. [Contract](../../../doc/STREAM_PCM_READER.md). |
| [`fvm_swf_stream_sound_snapshot_block_at`](../../../include/flashvm/swf_stream_sound.h#L131) | Borrow indexed block metadata/data while snapshot lives; cursor uses only copied frame/count for reporting. Не хранит data после закрытия. |
| [`fvm_audio_channel_create`](../../../include/flashvm/multimedia.h#L82) | Older modes copy host callbacks into a context-owned channel; cursor mode bypasses registration. Состояние FILE живёт до context cleanup. |
| [`fvm_swf_stream_sound_block_at`](../../../include/flashvm/swf_stream_sound.h#L59) | Live mode borrows block while source context remains loaded. Не переживает reload. |
| [`fvm_swf_stream_sound_snapshot_read_pcm`](../../../include/flashvm/swf_stream_sound.h#L135) | Older prepared mode copies a chosen block range without source VM. Заполняет caller samples. |
| [`fvm_swf_stream_sound_read_pcm`](../../../include/flashvm/swf_stream_sound.h#L61) | Live mode reads source-relative range synchronously on VM owner. Без удержания borrowed payload. |
| [`fvm_audio_channel_write`](../../../include/flashvm/multimedia.h#L84) | Older modes synchronously borrow PCM through file callback; no implicit retry. Cursor calls C callback directly. |
| [`fvm_audio_stop_all`](../../../include/flashvm/multimedia.h#L92) | Older modes flush/close active registered file output before context cleanup. Не управляет cursor. |
| [`fvm_swf_stream_sound_snapshot_close`](../../../include/flashvm/swf_stream_sound.h#L160) | Close native source once all readers stop; null accepted. Не обращается к уничтоженной VM. |

All source/stream APIs above: [inspection contract](../../../doc/SWF_STREAM_SOUND.md).
No socket/audio device or worker is started by this tutorial. The separate
native thread test demonstrates independent cursors; full player scheduling,
codec, audible-device and reference-platform acceptance remains open.

| API | Conversion ownership / Владение преобразованием |
| --- | --- |
| [`fvm_swf_stream_converter_init`](../../../include/flashvm/swf_stream_convert.h#L47) | Borrow immutable snapshot and copy explicit output rate/channels without allocation. Snapshot lives until caller quiesces conversion. [Contract](../../../doc/STREAM_PCM_CONVERSION.md). |
| [`fvm_swf_stream_converter_next`](../../../include/flashvm/swf_stream_convert.h#L58) | Produce bounded S16 in caller storage; rational phase survives blocks, no VM/I/O. Error/EOF preserves output and state. [Contract](../../../doc/STREAM_PCM_CONVERSION.md). |

## Explicit conversion / Явное преобразование

`OUTPUT.pcm --convert` uses the native converter at 48000 Hz stereo after source
VM teardown. Rational phase survives the two nonempty source blocks and the empty
block marker. It exports four stereo pairs (16 raw native-endian bytes), without
filling the missing movie frame. Per-block report lines retain source lengths;
final stdout is `Exported 4 sample frames from 3 blocks.` Caller owns the file,
source snapshot and converter until completion. No device or real-time scheduling.
The converter holds each block's final value; linear interpolation has no
anti-alias filter. [Full EN contract](../../../doc/STREAM_PCM_CONVERSION.md).

`OUTPUT.pcm --convert` после уничтожения source VM выдаёт 48000 Hz stereo:
четыре стереопары, 16 raw native-endian байт. Фаза переносится между блоками;
пропуск movie frame не заполняется. Строки по блокам показывают source lengths,
итог — четыре output frames из трёх блоков. Caller владеет FILE/snapshot/converter
до завершения. Устройства и real-time scheduling нет; на конце блока значение
удерживается, anti-alias filter отсутствует. [RU контракт](../../../doc/STREAM_PCM_CONVERSION.ru.md).

```sh
./build-stream/flashvm_examples_multimedia_swf_stream_main OUTPUT.pcm --convert
```

| API | Queue ownership / Владение очередью |
| --- | --- |
| [`fvm_swf_stream_output_init`](../../../include/flashvm/swf_stream_output.h#L44) | Borrow converter, retry buffer and empty native FIFO; copy backend, pause on success, no allocation. [Contract](../../../doc/STREAM_PCM_OUTPUT.md). |
| [`fvm_swf_stream_output_pump`](../../../include/flashvm/swf_stream_output.h#L53) | Prepare/retry one block; all-or-none FIFO acceptance, no file writes inside callbacks. [Contract](../../../doc/STREAM_PCM_OUTPUT.md). |
| [`fvm_swf_stream_output_info`](../../../include/flashvm/swf_stream_output.h#L72) | Observe FIFO backlog and copy source metadata/dequeue clock; EOF is separate from drain. [Contract](../../../doc/STREAM_PCM_OUTPUT.md). |
| [`fvm_swf_stream_output_pause`](../../../include/flashvm/swf_stream_output.h#L68) | Explicitly resume/stop this host's consumer without clearing or priming data. [Contract](../../../doc/STREAM_PCM_OUTPUT.md). |

## Bounded FIFO / Ограниченная FIFO

`OUTPUT.pcm --queue` uses [queue.inc](queue.inc): a two-frame caller-owned FIFO,
all-or-none native writes and an explicit serial file consumer. Conversion and
retry state never perform partial file writes; the consumer aborts on a file error.
All native objects outlive source VM teardown and remain owned until quiescence.
EOF is separate from draining accepted data. The file/report match `--convert`
exactly, including empty source markers. No audio device, worker or clock starts.
[EN contract](../../../doc/STREAM_PCM_OUTPUT.md).

`OUTPUT.pcm --queue` использует native FIFO на два frames: backend write атомарно
копирует данные, отдельный host consumer пишет файл. Частичная ошибка файла
прерывает экспорт, а не повторяется producer. Source VM уже уничтожена; native
объекты живут до quiescence. EOF не заменяет drain; файл/отчёт совпадают с
`--convert`, включая пустые markers. Audio device/thread/clock не запускаются.
[RU контракт](../../../doc/STREAM_PCM_OUTPUT.ru.md).

```sh
./build-stream/flashvm_examples_multimedia_swf_stream_main OUTPUT.pcm --queue
```

| API | Timeline ownership / Владение таймлайном |
| --- | --- |
| [`fvm_swf_stream_output_next_frame`](../../../include/flashvm/swf_stream_output.h#L57) | Query next pending/indexed source frame without callbacks or decoding; distinguish future-frame hold from FIFO pressure. [Contract](../../../doc/STREAM_PCM_OUTPUT.md). |
| [`fvm_swf_stream_output_pump_until`](../../../include/flashvm/swf_stream_output.h#L64) | Authorize only actual root frames; future data stays untouched, no implicit backward reset. [Contract](../../../doc/STREAM_PCM_OUTPUT.md). |
| [`fvm_standard_install`](../../../include/flashvm/standard.h#L20) | Install MovieClip methods on the owner before importing the controlled timeline. Managed prototypes remain context-owned. [Contract](../../../doc/STREAM_PCM_OUTPUT.md). |
| [`fvm_movieclip_load_swf_ex`](../../../include/flashvm/movieclip.h#L151) | Import exact same source with explicit stream omission; native snapshot/FIFO handle PCM separately. Returned clip is borrowed until pinned. [Contract](../../../doc/STREAM_PCM_OUTPUT.md). |
| [`fvm_gc_pin`](../../../include/flashvm/gc.h#L48) | Retain managed root/library through explicit owner GC safe points; cleanup owns one unpin. [Contract](../../../doc/STREAM_PCM_OUTPUT.md). |
| [`fvm_gc_unpin`](../../../include/flashvm/gc.h#L54) | Release only a successful clip pin before context destruction, after producer quiescence. [Contract](../../../doc/STREAM_PCM_OUTPUT.md). |
| [`fvm_movieclip_advance`](../../../include/flashvm/movieclip.h#L125) | Advance actual owner timeline; stopped ticks do not authorize later PCM. Never called from backend/worker callbacks. [Contract](../../../doc/STREAM_PCM_OUTPUT.md). |
| [`fvm_object_get`](../../../include/flashvm/value.h#L72) | Read actual _currentframe on owner, validate copied scalar and authorize native output; may enter object semantics, never worker-safe VM access. [Contract](../../../doc/STREAM_PCM_OUTPUT.md). |
| [`fvm_object_call`](../../../include/flashvm/value.h#L126) | Controlled stop/play calls demonstrate explicit MovieClip pause/resume outside audio callbacks. Effects are not rolled back on later failure. [Contract](../../../doc/STREAM_PCM_OUTPUT.md). |
| [`fvm_gc_step`](../../../include/flashvm/gc.h#L90) | Owner safe point between completed ticks; pinned root survives, no callback/worker GC. [Contract](../../../doc/STREAM_PCM_OUTPUT.md). |

## Actual timeline / Настоящий таймлайн

`OUTPUT.pcm --timeline` keeps a pinned MovieClip on the VM owner and authorizes
native FIFO data from actual _currentframe. It exercises stop/two stopped ticks/
play, source frame gaps and owner GC safe points; no future or duplicate PCM.
File/report equal `--queue` exactly. This mode retains the source VM deliberately;
native-only modes keep their teardown demonstration. No backend callback enters
VM and no audio device is opened. [EN contract](../../../doc/STREAM_PCM_OUTPUT.md).

`OUTPUT.pcm --timeline` сохраняет pinned MovieClip на owner и разрешает native
FIFO данные по настоящему _currentframe. Проверяются stop, два stopped ticks,
play, source gaps и owner GC safe points. Будущий/повторный PCM не выдаётся;
файл/отчёт совпадают с `--queue`. Source VM здесь намеренно жива, native-only
режимы сохраняют демонстрацию teardown. Callback не входит в VM, audio device
не открывается. [RU контракт](../../../doc/STREAM_PCM_OUTPUT.ru.md).

```sh
./build-stream/flashvm_examples_multimedia_swf_stream_main OUTPUT.pcm --timeline
```

Backward navigation requires source seek/queue clear and new priming; the separate
native AVM1 interpreter/JIT integration test covers that policy. This is a controlled
host tutorial, not complete SWF streaming audio scheduling or A/V synchronization.
## Prepared PCM normalization (v0.763) / Нормализация подготовленного PCM

Prepared modes now call [`fvm_swf_stream_sound_snapshot_open_pcm`](../../../include/flashvm/swf_stream_sound.h#L87): PCM is copied
unchanged; whole-packet ADPCM can be expanded before worker publication. This
embedded tutorial still uses its original PCM fixture and preserves exact output.
The compressed stream contract and independent FFmpeg checks are documented in
[English](../../../doc/STREAM_ADPCM.md) / [Русский](../../../doc/STREAM_ADPCM.ru.md).

Подготовленные режимы используют [`fvm_swf_stream_sound_snapshot_open_pcm`](../../../include/flashvm/swf_stream_sound.h#L87):
исходный PCM копируется без изменения, целые пакеты ADPCM можно заранее раскрыть.
Встроенный пример сохраняет исходный PCM и прежний точный результат. Декодирование
выполняется до уничтожения исходной VM; чтение подготовленных данных — после.

| API | Contract / Контракт |
| --- | --- |
| [`fvm_swf_stream_sound_snapshot_open_pcm`](../../../include/flashvm/swf_stream_sound.h#L87) | Owner prepares an independent immutable PCM snapshot; may allocate/decode, clears output on failure, normalizes whole ADPCM packets to native S16 and requires explicit close. Владелец подготавливает независимый PCM до worker; ошибка освобождает временные данные. |
## Explicit storage budget (v0.766) / Явный бюджет памяти

Since v0.768 all prepared modes of this unchanged-PCM example use [`fvm_swf_stream_sound_snapshot_open_bounded`](../../../include/flashvm/swf_stream_sound.h#L72) with
[TUTORIAL_STREAM_SOURCE_BYTES](example_constants.h#L10) (one MiB). The additional `--storage` mode exports
unchanged native PCM after input VM teardown and first prints
`Storage: retained N bytes, peak M bytes.` Values depend on native structure sizes;
this tiny PCM fixture has equal retained and peak charges, not a fixed ABI byte count.
The output file is still replaced only by the explicit file consumer. Missing output
paths fail with ordinary cleanup, after printing the storage observation.

Подготовленные режимы ограничены одним МиБ, включая объект/индекс/временный пик.
`--storage` запрашивает память после уничтожения входной VM и экспортирует тот же
PCM; `N`/`M` зависят от размеров нативных структур. Для данного PCM peak равен
retained. Это учёт владения, не RSS процесса. Предел задаётся в
[example_constants.h](example_constants.h); подробный контракт: [EN](../../../doc/STREAM_STORAGE.md) /
[RU](../../../doc/STREAM_STORAGE.ru.md).

```sh
./build-stream/flashvm_examples_multimedia_swf_stream_main OUTPUT.pcm --storage
```

| API | Contract / Контракт |
| --- | --- |
| [`fvm_swf_stream_sound_snapshot_open_bounded`](../../../include/flashvm/swf_stream_sound.h#L72) | Owner copies encoded PCM under an explicit peak ceiling without normalization; clears output and releases temporaries on failure, closes via the ordinary snapshot API. Владелец ограничивает пик до публикации worker; бюджет не изменяет runtime config. |
| [`fvm_swf_stream_sound_snapshot_storage_info`](../../../include/flashvm/swf_stream_sound.h#L123) | Copies retained/peak charges without allocation or VM access; works after VM teardown, preserves output for null/wrong-size inputs. Snapshot must remain alive until return; close after all readers. |
The fixture is already PCM. This raw bounded preparation preserves its representation;
use the separate normalization API when preparing supported ADPCM for a native worker.
Пример уже содержит PCM: raw-вход сохраняет исходные байты; для ADPCM перед worker
применяется отдельная нормализация. [Full ownership contract / контракт](../../../doc/STREAM_STORAGE.md#encoded-copies-without-decoding-v0768).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_audio_channel_create` | function | [include/flashvm/multimedia.h:82](../../../include/flashvm/multimedia.h#L82) |
| `fvm_audio_channel_write` | function | [include/flashvm/multimedia.h:84](../../../include/flashvm/multimedia.h#L84) |
| `fvm_audio_stop_all` | function | [include/flashvm/multimedia.h:92](../../../include/flashvm/multimedia.h#L92) |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:62](../../../include/flashvm/flashvm.h#L62) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:68](../../../include/flashvm/flashvm.h#L68) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_gc_pin` | function | [include/flashvm/gc.h:48](../../../include/flashvm/gc.h#L48) |
| `fvm_gc_step` | function | [include/flashvm/gc.h:90](../../../include/flashvm/gc.h#L90) |
| `fvm_gc_unpin` | function | [include/flashvm/gc.h:54](../../../include/flashvm/gc.h#L54) |
| `fvm_load_memory` | function | [include/flashvm/flashvm.h:72](../../../include/flashvm/flashvm.h#L72) |
| `FVM_MediaOutput` | struct | [include/flashvm/multimedia.h:68](../../../include/flashvm/multimedia.h#L68) |
| `fvm_movieclip_advance` | function | [include/flashvm/movieclip.h:125](../../../include/flashvm/movieclip.h#L125) |
| `fvm_movieclip_load_swf_ex` | function | [include/flashvm/movieclip.h:151](../../../include/flashvm/movieclip.h#L151) |
| `fvm_object_call` | function | [include/flashvm/value.h:126](../../../include/flashvm/value.h#L126) |
| `fvm_object_get` | function | [include/flashvm/value.h:72](../../../include/flashvm/value.h#L72) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../../../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../../../include/flashvm/flashvm.h#L66) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:20](../../../include/flashvm/standard.h#L20) |
| `fvm_swf_stream_converter_init` | function | [include/flashvm/swf_stream_convert.h:47](../../../include/flashvm/swf_stream_convert.h#L47) |
| `fvm_swf_stream_converter_next` | function | [include/flashvm/swf_stream_convert.h:58](../../../include/flashvm/swf_stream_convert.h#L58) |
| `fvm_swf_stream_output_info` | function | [include/flashvm/swf_stream_output.h:72](../../../include/flashvm/swf_stream_output.h#L72) |
| `fvm_swf_stream_output_init` | function | [include/flashvm/swf_stream_output.h:44](../../../include/flashvm/swf_stream_output.h#L44) |
| `fvm_swf_stream_output_next_frame` | function | [include/flashvm/swf_stream_output.h:57](../../../include/flashvm/swf_stream_output.h#L57) |
| `fvm_swf_stream_output_pause` | function | [include/flashvm/swf_stream_output.h:68](../../../include/flashvm/swf_stream_output.h#L68) |
| `fvm_swf_stream_output_pump` | function | [include/flashvm/swf_stream_output.h:53](../../../include/flashvm/swf_stream_output.h#L53) |
| `fvm_swf_stream_output_pump_until` | function | [include/flashvm/swf_stream_output.h:64](../../../include/flashvm/swf_stream_output.h#L64) |
| `fvm_swf_stream_pcm_reader_init` | function | [include/flashvm/swf_stream_pcm.h:40](../../../include/flashvm/swf_stream_pcm.h#L40) |
| `fvm_swf_stream_pcm_reader_next` | function | [include/flashvm/swf_stream_pcm.h:48](../../../include/flashvm/swf_stream_pcm.h#L48) |
| `fvm_swf_stream_sound_at` | function | [include/flashvm/swf_stream_sound.h:55](../../../include/flashvm/swf_stream_sound.h#L55) |
| `fvm_swf_stream_sound_block_at` | function | [include/flashvm/swf_stream_sound.h:59](../../../include/flashvm/swf_stream_sound.h#L59) |
| `fvm_swf_stream_sound_block_count` | function | [include/flashvm/swf_stream_sound.h:57](../../../include/flashvm/swf_stream_sound.h#L57) |
| `fvm_swf_stream_sound_count` | function | [include/flashvm/swf_stream_sound.h:53](../../../include/flashvm/swf_stream_sound.h#L53) |
| `fvm_swf_stream_sound_read_pcm` | function | [include/flashvm/swf_stream_sound.h:61](../../../include/flashvm/swf_stream_sound.h#L61) |
| `fvm_swf_stream_sound_snapshot_block_at` | function | [include/flashvm/swf_stream_sound.h:131](../../../include/flashvm/swf_stream_sound.h#L131) |
| `fvm_swf_stream_sound_snapshot_block_count` | function | [include/flashvm/swf_stream_sound.h:129](../../../include/flashvm/swf_stream_sound.h#L129) |
| `fvm_swf_stream_sound_snapshot_close` | function | [include/flashvm/swf_stream_sound.h:160](../../../include/flashvm/swf_stream_sound.h#L160) |
| `fvm_swf_stream_sound_snapshot_info` | function | [include/flashvm/swf_stream_sound.h:127](../../../include/flashvm/swf_stream_sound.h#L127) |
| `fvm_swf_stream_sound_snapshot_open` | function | [include/flashvm/swf_stream_sound.h:65](../../../include/flashvm/swf_stream_sound.h#L65) |
| `fvm_swf_stream_sound_snapshot_open_bounded` | function | [include/flashvm/swf_stream_sound.h:72](../../../include/flashvm/swf_stream_sound.h#L72) |
| `fvm_swf_stream_sound_snapshot_open_pcm` | function | [include/flashvm/swf_stream_sound.h:87](../../../include/flashvm/swf_stream_sound.h#L87) |
| `fvm_swf_stream_sound_snapshot_read_pcm` | function | [include/flashvm/swf_stream_sound.h:135](../../../include/flashvm/swf_stream_sound.h#L135) |
| `fvm_swf_stream_sound_snapshot_storage_info` | function | [include/flashvm/swf_stream_sound.h:123](../../../include/flashvm/swf_stream_sound.h#L123) |
<!-- source-links:end -->
