# Incremental StartSound mixing / Порционный микшер StartSound

## Назначение / Purpose

Исполняемый C89 учебник показывает три software voices: две обычные копии ID 7
и одну ID 8. SyncNoMultiple подавляет четвёртый голос, STOP удаляет обе копии
ID 7, а ID 8 продолжает играть после уничтожения source context/runtime.
Микшер насыщает сумму один раз и дополняет конец тишиной. Это подготовка своего
моста/очереди аудиовывода: полученный буфер можно передавать отдельному host output.

This runnable C89 example demonstrates overlapping event voices, no-multiple,
resource-selective stopping, one-pass saturation and native lifetime. It uses
explicit sample reads, not a hidden MovieClip/device clock. A successful software
run does not demonstrate audible hardware playback.

## Требования и запуск / Build and run

Из корня репозитория (или измените `-S` при отдельном распространении FlashVM):

```sh
cmake -S flashVM -B /tmp/flashvm-build -DFVM_BUILD_EXAMPLES=ON
cmake --build /tmp/flashvm-build --target flashvm_examples_multimedia_swf_event_mixer_main
/tmp/flashvm-build/flashvm_examples_multimedia_swf_event_mixer_main --demo
/tmp/flashvm-build/flashvm_examples_multimedia_swf_event_mixer_main --demo result.pcm
```

Второй запуск создаёт/заменяет только явно названный файл. Без output аргумента
файлы и устройства не открываются. Без `--demo` выводится usage; неверные аргументы
дают exit 2, API/I/O errors — exit 1, успех — 0. Достаточно NO_MEDIA; FFmpeg,
SDL, ALSA, микрофон и разрешения ОС не нужны. CMake-цель доступна при examples ON.
All supported hosts use the same software API; compile and run natively on each
OS before claiming native acceptance. This checkpoint was exercised on Linux.

Ожидаемый stdout:

```text
first=-32768 tail=32512 active=0
```

Optional result.pcm contains four native signed-16 mono samples:
-32768, 32512, 0, 0 (8 bytes on the supported short representation). It has no WAV
header. Its declared nominal clock is 44100 Hz; such a tiny diagnostic sequence
is not a meaningful audio listening test. Windows paths follow the ordinary C
runtime in this tutorial; the separate desktop launcher has its own UTF-8 adapter.

## Реализация и владение / Implementation and ownership

[main.c](main.c) хранит читаемый literal SWF8 с точным размером и пояснениями
каждого тега; [example_constants.h](example_constants.h) называет события и
геометрию отдельно. Каждый непустой кодовый ряд прокомментирован, assert нет.
VM загружает source dictionary; mixer_submit копирует native PCM/огибающие.
Ни один managed pointer не сохраняется. После первого read и STOP источник
уничтожается; tail читается независимо. Слоты/голоса освобождаются close и после
исчерпания, FILE закрывается даже при частичной ошибке. Ошибка fclose меняет
результат на failure. Никакого rollback уже записанного файла нет.

Each U8 source is -32768, 32512 after conversion. Three first samples sum to
-98304 and saturate to -32768. The ID-7 stop then leaves only ID 8's second sample;
two more requested frames are silence. No-multiple is applied before decoding a
new voice. The active count is zero after tail exhaustion. Calls are serialized
on main thread; real-time device callbacks should consume a separate host queue.

## Демонстрируемый API / API demonstrated

| Function | Individual behavior and detailed contract |
| --- | --- |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h#L62) | Инициализирует caller config; [EMBEDDING_API.md](../../../doc/EMBEDDING_API.md), создание runtime. |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L64) | Создаёт владеющий runtime; освободить после context: [EMBEDDING_API.md](../../../doc/EMBEDDING_API.md). |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h#L68) | Создаёт source VM в runtime; владеющий поток: [EMBEDDING_API.md](../../../doc/EMBEDDING_API.md). |
| [`fvm_load_memory`](../../../include/flashvm/flashvm.h#L72) | Загружает/копирует SWF; исходный literal не становится borrowed mixer storage: [EMBEDDING_API.md](../../../doc/EMBEDDING_API.md). |
| [`fvm_swf_sound_mixer_create`](../../../include/flashvm/swf_sound_mixer.h#L22) | Создаёт фиксированную геометрию и таблицу слотов, очищает out при отказе: [SWF_SOUND_MIXER.md](../../../doc/SWF_SOUND_MIXER.md#fvm_swf_sound_mixer_create). |
| [`fvm_swf_sound_mixer_submit`](../../../include/flashvm/swf_sound_mixer.h#L27) | Синхронно готовит голос или выполняет stop/no-multiple; может выделять/декодировать: [контракт](../../../doc/SWF_SOUND_MIXER.md#fvm_swf_sound_mixer_submit). |
| [`fvm_swf_sound_mixer_read`](../../../include/flashvm/swf_sound_mixer.h#L32) | Продвигает software clock, выдаёт точный S16 объём с тишиной и единственным насыщением, без новых выделений: [контракт](../../../doc/SWF_SOUND_MIXER.md#fvm_swf_sound_mixer_read). |
| [`fvm_swf_sound_mixer_info`](../../../include/flashvm/swf_sound_mixer.h#L36) | Копирует activity/geometry без продвижения и без VM: [контракт](../../../doc/SWF_SOUND_MIXER.md#fvm_swf_sound_mixer_info). |
| [`fvm_swf_sound_mixer_stop_all`](../../../include/flashvm/swf_sound_mixer.h#L38) | Освобождает все software voices; не сбрасывает устройство/FILE: [контракт](../../../doc/SWF_SOUND_MIXER.md#fvm_swf_sound_mixer_stop_all). |
| [`fvm_swf_sound_mixer_close`](../../../include/flashvm/swf_sound_mixer.h#L40) | Освобождает native owner; null допустим, source VM не нужна: [контракт](../../../doc/SWF_SOUND_MIXER.md#fvm_swf_sound_mixer_close). |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L70) | Освобождает исходный context; native mixer продолжает работать: [EMBEDDING_API.md](../../../doc/EMBEDDING_API.md). |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L66) | Завершает source runtime после context: [EMBEDDING_API.md](../../../doc/EMBEDDING_API.md). |

Полное русское руководство: [SWF_SOUND_MIXER.ru.md](../../../doc/SWF_SOUND_MIXER.ru.md).
[Описание в общем каталоге](../../README.md#example-multimedia-swf_event_mixer).

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
| `fvm_swf_sound_mixer_close` | function | [include/flashvm/swf_sound_mixer.h:40](../../../include/flashvm/swf_sound_mixer.h#L40) |
| `fvm_swf_sound_mixer_create` | function | [include/flashvm/swf_sound_mixer.h:22](../../../include/flashvm/swf_sound_mixer.h#L22) |
| `fvm_swf_sound_mixer_info` | function | [include/flashvm/swf_sound_mixer.h:36](../../../include/flashvm/swf_sound_mixer.h#L36) |
| `fvm_swf_sound_mixer_read` | function | [include/flashvm/swf_sound_mixer.h:32](../../../include/flashvm/swf_sound_mixer.h#L32) |
| `fvm_swf_sound_mixer_stop_all` | function | [include/flashvm/swf_sound_mixer.h:38](../../../include/flashvm/swf_sound_mixer.h#L38) |
| `fvm_swf_sound_mixer_submit` | function | [include/flashvm/swf_sound_mixer.h:27](../../../include/flashvm/swf_sound_mixer.h#L27) |
<!-- source-links:end -->
