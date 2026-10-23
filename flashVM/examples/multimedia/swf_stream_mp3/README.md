# swf_stream_mp3 — encoded blocks after VM teardown / Сжатые блоки после уничтожения VM

## Purpose / Назначение

Executable, fully annotated C89 tutorial: inspect one live MP3 block, capture its
encoded segment, destroy the context/runtime and inspect both indexed blocks.
This is useful when a host wants to extract encoded data for its own decoder,
retain an immutable diagnostic sample or inspect stream synchronization fields
without keeping the whole VM alive. Source: [main.c](main.c).

Исполняемый C89 пример показывает live-запрос MP3-префикса, независимый снимок
сегмента и чтение после уничтожения context/runtime. Полезен для передачи
закодированных байтов собственному декодеру, независимого анализа и диагностики.
Каждая непустая строка прокомментирована; assertions не используются.

## Preparation / Подготовка

From the `flashVM` project directory / Из папки проекта `flashVM`:

```sh
cmake -S . -B build-stream -DFVM_DEFS_FILE=cmake/profiles/minimal.defs -DCMAKE_BUILD_TYPE=Release
cmake --build build-stream --target flashvm_examples_multimedia_swf_stream_mp3_main --parallel
./build-stream/flashvm_examples_multimedia_swf_stream_mp3_main --demo
```

Requires a C89/C++98 toolchain; NO_MEDIA/NO_GRAPHICS are sufficient. No FFmpeg,
audio device, microphone permission, external SWF or output file is needed.
Without arguments it prints usage only. Unknown arguments fail before allocation.
Windows/macOS/Android need their normal target toolchain/launcher; Linux validation
here does not establish native execution on those platforms.

Достаточно C89/C++98 и минимального профиля. FFmpeg, устройства, разрешения,
внешние SWF и выходные файлы не нужны. Без аргументов — только usage; неверные
параметры отклоняются до создания VM. Нативная проверка других ОС не заявляется.

## Expected result / Результат

```text
header latency=-2
frame=1 declared=576 seek=-3 bytes=3
frame=2 declared=0 seek=0 bytes=0
```

The fixture contains three deliberately opaque MPEG bytes, **not playable MP3**.
It verifies metadata/lifetime behavior, not codec decoding or audible playback.
The second block has a complete four-byte prefix and an empty MPEG span. Declared
samples are copied wire metadata; even 576 does not certify an actual decoded
frame. Header latency and block seek stay separate; the tutorial never adds them.

Встроенные три MPEG-байта намеренно не являются воспроизводимым MP3. Пример
показывает метаданные и владение, не декодирование и не звук. У второго блока
полный префикс, но нет MPEG-данных. Объявленное число samples не удостоверяет
длину аудио. Header latency и seek блока выводятся отдельно без суммирования.

## Implementation and ownership / Реализация и владение

`main` initializes owners to null, loads the fixture, checks the live view and
copies one segment. Source context/runtime are explicitly destroyed before the
snapshot header/count/prefix requests. Borrowed MPEG views are used while the
snapshot is open, then invalidated by close. No thread sharing or implicit output
is performed. Every failure goes through one cleanup path and returns nonzero;
no owner is destroyed twice. `print_block` only reads scalar metadata.

`main` обнуляет owners, загружает fixture, проверяет live view и копирует сегмент.
Исходная VM уничтожается до запросов к снимку. Borrowed views используются только
до close. Общего доступа между потоками и скрытого воспроизведения нет. Ошибки
обрабатываются обычным cleanup; повторного уничтожения owners нет.

## Demonstrated API / Демонстрируемые функции

| Function | Behavior demonstrated and manual |
| --- | --- |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h#L62) | Initializes configuration/ABI before runtime creation; [API_INDEX.md](../../../doc/API_INDEX.md#runtime-and-context) |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L64) | Acquires the source runtime owner; [API_INDEX.md](../../../doc/API_INDEX.md#runtime-and-context) |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h#L68) | Acquires the source movie context; [API_INDEX.md](../../../doc/API_INDEX.md#runtime-and-context) |
| [`fvm_load_memory`](../../../include/flashvm/flashvm.h#L72) | Loads canonical native SWF storage; successful reload would end live views; [API_INDEX.md](../../../doc/API_INDEX.md#runtime-and-context) |
| [`fvm_swf_stream_sound_mp3_block_at`](../../../include/flashvm/swf_stream_sound.h#L51) | Validates selected live prefix and borrows MPEG span without allocation; [SWF_STREAM_MP3.md](../../../doc/SWF_STREAM_MP3.md#fvm_swf_stream_sound_mp3_block_at) |
| [`fvm_swf_stream_sound_snapshot_open`](../../../include/flashvm/swf_stream_sound.h#L65) | Allocates/copies one encoded segment under the native budget; [SWF_STREAM_MP3.md](../../../doc/SWF_STREAM_MP3.md#capture-and-ownership) |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L70) | Ends source context/live views before reading the snapshot; [API_INDEX.md](../../../doc/API_INDEX.md#runtime-and-context) |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L66) | Releases the source runtime; snapshot retains no VM owner; [API_INDEX.md](../../../doc/API_INDEX.md#runtime-and-context) |
| [`fvm_swf_stream_sound_snapshot_info`](../../../include/flashvm/swf_stream_sound.h#L165) | Copies header latency independently of block seek; [SWF_STREAM_SOUND.md](../../../doc/SWF_STREAM_SOUND.md#prepared-immutable-pcm-index--подготовленный-неизменяемый-индекс-pcm) |
| [`fvm_swf_stream_sound_snapshot_block_count`](../../../include/flashvm/swf_stream_sound.h#L167) | Reads the prepared index without movie rescanning; [SWF_STREAM_SOUND.md](../../../doc/SWF_STREAM_SOUND.md#prepared-immutable-pcm-index--подготовленный-неизменяемый-индекс-pcm) |
| [`fvm_swf_stream_sound_snapshot_mp3_block_at`](../../../include/flashvm/swf_stream_sound.h#L171) | Borrows MPEG span and copied prefix after source VM teardown; [SWF_STREAM_MP3.md](../../../doc/SWF_STREAM_MP3.md#fvm_swf_stream_sound_snapshot_mp3_block_at) |
| [`fvm_swf_stream_sound_snapshot_close`](../../../include/flashvm/swf_stream_sound.h#L198) | Frees native storage and ends all snapshot views; accepts null; [SWF_STREAM_MP3.md](../../../doc/SWF_STREAM_MP3.md#capture-and-ownership) |

Каталог: [examples/README.md](../../README.md#example-multimedia-swf_stream_mp3).


## Real PCM export / Экспорт настоящего PCM (v0.810)

```sh
./build-stream/flashvm_examples_multimedia_swf_stream_mp3_main --decode INPUT.swf OUTPUT.pcm
```

[decode.inc](decode.inc) reads a seekable SWF of at most 64 MiB, fully prepares its
first supported audio segment with a separate 64 MiB native peak ceiling, destroys
the VM/runtime, then writes bounded native-endian interleaved S16 chunks. Output
is replaced only after successful preparation; write/flush errors fail and may
leave a partial file. Exactly one segment is exported; no playback, resampling,
A/V scheduling or cross-segment concatenation is implied. PCM, ADPCM and supported
MP3, including free-format, share the same preparation contract.

Режим читает seekable SWF не больше 64 МиБ, полностью подготавливает первый
поддерживаемый аудиосегмент с отдельным ограничением пика 64 МиБ, уничтожает
VM/runtime и пишет native-endian interleaved S16. Выход заменяется только после
успешной подготовки; ошибка записи/закрытия может оставить частичный файл.
Экспортируется один сегмент без воспроизведения, пересчёта частоты и A/V-таймлайна.

| API | Contract / Контракт |
| --- | --- |
| [`fvm_swf_stream_sound_snapshot_open_pcm_bounded`](../../../include/flashvm/swf_stream_sound.h#L102) | Validate and normalize the complete first segment on the VM owner thread, outside GC; errors publish no owner. Caller closes the independent snapshot. Peak limit includes transient encoded and PCM storage. [EN](../../../doc/STREAM_STORAGE.md) / [RU](../../../doc/STREAM_STORAGE.ru.md). |
| [`fvm_swf_stream_pcm_reader_init`](../../../include/flashvm/swf_stream_pcm.h#L40) | Borrow a prepared immutable PCM snapshot without allocation; caller cursor lives independently after VM teardown and cannot outlive the snapshot. Errors preserve cursor. [Contract](../../../doc/STREAM_PCM_READER.md). |
| [`fvm_swf_stream_pcm_reader_next`](../../../include/flashvm/swf_stream_pcm.h#L48) | Fill bounded caller S16 storage and block/frame metadata without VM, allocation or I/O; zero markers advance and EOF preserves outputs. Cursor has one serial owner. [Contract](../../../doc/STREAM_PCM_READER.md). |


## Verify without an output file / Проверка без выходного файла (v0.818)

```sh
./build-stream/flashvm_examples_multimedia_swf_stream_mp3_main --verify INPUT.swf
```

This mode uses the same bounded first-segment normalization and detached native
reader as export, then reports `Verified N frames at RATE Hz, CHANNELS channels`.
It opens no destination and writes no PCM. Protected MP3 CRC errors fail before
reporting success; mute/metadata-only inspection is a different operation. Input
remains capped at 64 MiB and preparation at a separate 64 MiB peak. Only the first
audio segment is checked, not an entire movie's timeline or every sound resource.

Режим использует ту же ограниченную нормализацию первого сегмента и независимый
нативный reader; сообщает `Verified N frames at RATE Hz, CHANNELS channels`.
Выходной файл не открывается, PCM не записывается. Плохая защищённая MP3 CRC
даёт ошибку до успешного отчёта. Вход и пик подготовки отдельно ограничены 64 МиБ.
Проверяется только первый аудиосегмент, а не весь таймлайн и все ресурсы фильма.

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
| `fvm_swf_stream_pcm_reader_init` | function | [include/flashvm/swf_stream_pcm.h:40](../../../include/flashvm/swf_stream_pcm.h#L40) |
| `fvm_swf_stream_pcm_reader_next` | function | [include/flashvm/swf_stream_pcm.h:48](../../../include/flashvm/swf_stream_pcm.h#L48) |
| `fvm_swf_stream_sound_mp3_block_at` | function | [include/flashvm/swf_stream_sound.h:51](../../../include/flashvm/swf_stream_sound.h#L51) |
| `fvm_swf_stream_sound_snapshot_block_count` | function | [include/flashvm/swf_stream_sound.h:167](../../../include/flashvm/swf_stream_sound.h#L167) |
| `fvm_swf_stream_sound_snapshot_close` | function | [include/flashvm/swf_stream_sound.h:198](../../../include/flashvm/swf_stream_sound.h#L198) |
| `fvm_swf_stream_sound_snapshot_info` | function | [include/flashvm/swf_stream_sound.h:165](../../../include/flashvm/swf_stream_sound.h#L165) |
| `fvm_swf_stream_sound_snapshot_mp3_block_at` | function | [include/flashvm/swf_stream_sound.h:171](../../../include/flashvm/swf_stream_sound.h#L171) |
| `fvm_swf_stream_sound_snapshot_open` | function | [include/flashvm/swf_stream_sound.h:65](../../../include/flashvm/swf_stream_sound.h#L65) |
| `fvm_swf_stream_sound_snapshot_open_pcm_bounded` | function | [include/flashvm/swf_stream_sound.h:102](../../../include/flashvm/swf_stream_sound.h#L102) |
<!-- source-links:end -->
