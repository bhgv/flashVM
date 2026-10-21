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
| [`fvm_swf_stream_sound_snapshot_info`](../../../include/flashvm/swf_stream_sound.h#L127) | Copies header latency independently of block seek; [SWF_STREAM_SOUND.md](../../../doc/SWF_STREAM_SOUND.md#prepared-immutable-pcm-index--подготовленный-неизменяемый-индекс-pcm) |
| [`fvm_swf_stream_sound_snapshot_block_count`](../../../include/flashvm/swf_stream_sound.h#L129) | Reads the prepared index without movie rescanning; [SWF_STREAM_SOUND.md](../../../doc/SWF_STREAM_SOUND.md#prepared-immutable-pcm-index--подготовленный-неизменяемый-индекс-pcm) |
| [`fvm_swf_stream_sound_snapshot_mp3_block_at`](../../../include/flashvm/swf_stream_sound.h#L133) | Borrows MPEG span and copied prefix after source VM teardown; [SWF_STREAM_MP3.md](../../../doc/SWF_STREAM_MP3.md#fvm_swf_stream_sound_snapshot_mp3_block_at) |
| [`fvm_swf_stream_sound_snapshot_close`](../../../include/flashvm/swf_stream_sound.h#L160) | Frees native storage and ends all snapshot views; accepts null; [SWF_STREAM_MP3.md](../../../doc/SWF_STREAM_MP3.md#capture-and-ownership) |

Каталог: [examples/README.md](../../README.md#example-multimedia-swf_stream_mp3).

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
| `fvm_swf_stream_sound_mp3_block_at` | function | [include/flashvm/swf_stream_sound.h:51](../../../include/flashvm/swf_stream_sound.h#L51) |
| `fvm_swf_stream_sound_snapshot_block_count` | function | [include/flashvm/swf_stream_sound.h:129](../../../include/flashvm/swf_stream_sound.h#L129) |
| `fvm_swf_stream_sound_snapshot_close` | function | [include/flashvm/swf_stream_sound.h:160](../../../include/flashvm/swf_stream_sound.h#L160) |
| `fvm_swf_stream_sound_snapshot_info` | function | [include/flashvm/swf_stream_sound.h:127](../../../include/flashvm/swf_stream_sound.h#L127) |
| `fvm_swf_stream_sound_snapshot_mp3_block_at` | function | [include/flashvm/swf_stream_sound.h:133](../../../include/flashvm/swf_stream_sound.h#L133) |
| `fvm_swf_stream_sound_snapshot_open` | function | [include/flashvm/swf_stream_sound.h:65](../../../include/flashvm/swf_stream_sound.h#L65) |
<!-- source-links:end -->
