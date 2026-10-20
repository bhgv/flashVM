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
| [`fvm_swf_sound_mixer_create`](../../../include/flashvm/swf_sound_mixer.h#L45) | Создаёт фиксированную геометрию и таблицу слотов, очищает out при отказе: [SWF_SOUND_MIXER.md](../../../doc/SWF_SOUND_MIXER.md#fvm_swf_sound_mixer_create). |
| [`fvm_swf_sound_mixer_submit`](../../../include/flashvm/swf_sound_mixer.h#L50) | Синхронно готовит голос или выполняет stop/no-multiple; может выделять/декодировать: [контракт](../../../doc/SWF_SOUND_MIXER.md#fvm_swf_sound_mixer_submit). |
| [`fvm_swf_sound_mixer_read`](../../../include/flashvm/swf_sound_mixer.h#L55) | Продвигает software clock, выдаёт точный S16 объём с тишиной и единственным насыщением, без новых выделений: [контракт](../../../doc/SWF_SOUND_MIXER.md#fvm_swf_sound_mixer_read). |
| [`fvm_swf_sound_mixer_info`](../../../include/flashvm/swf_sound_mixer.h#L59) | Копирует activity/geometry без продвижения и без VM: [контракт](../../../doc/SWF_SOUND_MIXER.md#fvm_swf_sound_mixer_info). |
| [`fvm_swf_sound_mixer_stop_all`](../../../include/flashvm/swf_sound_mixer.h#L61) | Освобождает все software voices; не сбрасывает устройство/FILE: [контракт](../../../doc/SWF_SOUND_MIXER.md#fvm_swf_sound_mixer_stop_all). |
| [`fvm_swf_sound_mixer_close`](../../../include/flashvm/swf_sound_mixer.h#L63) | Освобождает native owner; null допустим, source VM не нужна: [контракт](../../../doc/SWF_SOUND_MIXER.md#fvm_swf_sound_mixer_close). |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L70) | Освобождает исходный context; native mixer продолжает работать: [EMBEDDING_API.md](../../../doc/EMBEDDING_API.md). |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L66) | Завершает source runtime после context: [EMBEDDING_API.md](../../../doc/EMBEDDING_API.md). |

Полное русское руководство: [SWF_SOUND_MIXER.ru.md](../../../doc/SWF_SOUND_MIXER.ru.md).
[Описание в общем каталоге](../../README.md#example-multimedia-swf_event_mixer).
## Actual timeline delivery / Реальная доставка кадра (v0.579)

```sh
/tmp/flashvm-build/flashvm_examples_multimedia_swf_event_mixer_main --timeline-demo
/tmp/flashvm-build/flashvm_examples_multimedia_swf_event_mixer_main --timeline-demo timeline.pcm
```

Этот режим устанавливает стандартные объекты, импортирует тот же SWF после
[fvm_load_memory](../../../include/flashvm/flashvm.h#L72), pin-ит root, регистрирует callback и вызывает один advance.
Все четыре StartSound приходят из реально исполняемого кадра: start 7,
no-multiple 7, stop 7, start 8. При первом PCM read остаётся только ID 8.
Поэтому stdout и четыре выходных sample совпадают с manual demo, но управляющий
путь другой. VM затем уничтожается, оставшийся PCM читается независимо.
Это не открытие устройства и не привязка к его часам. Прежний --demo сохраняет
показ перекрытия трёх вручную запущенных голосов. Выходной путь по-прежнему явный.

| Additional API | Individual contract / Где читать |
| --- | --- |
| [`fvm_standard_install`](../../../include/flashvm/standard.h#L20) | Устанавливает методы MovieClip до исполнения кадра: [API_INDEX.md](../../../doc/API_INDEX.md). |
| [`fvm_movieclip_load_swf`](../../../include/flashvm/movieclip.h#L76) | Импортирует кадры/корневой звук из совпадающего source; clip нужно удерживать: [MOVIECLIP_SOUND.md](../../../doc/MOVIECLIP_SOUND.md#loading-and-source-identity). |
| [`fvm_gc_pin`](../../../include/flashvm/gc.h#L48) | [GC:] Удерживает managed root во время host операций: [GC_NATIVE_ROOTS.md](../../../doc/GC_NATIVE_ROOTS.md). |
| [`fvm_movieclip_set_sound_callback`](../../../include/flashvm/movieclip.h#L21) | Заимствует callback/userdata, не сериализует их и не открывает device: [контракт](../../../doc/MOVIECLIP_SOUND.md#callback-registration-contract). |
| [`fvm_movieclip_advance`](../../../include/flashvm/movieclip.h#L99) | Реальный вход доставляет события перед DoAction; ошибки не откатываются: [порядок](../../../doc/MOVIECLIP_SOUND.md#execution-order-and-errors). |
| [`fvm_gc_unpin`](../../../include/flashvm/gc.h#L54) | [GC:] Снимает только успешно полученный pin перед уничтожением context: [GC_NATIVE_ROOTS.md](../../../doc/GC_NATIVE_ROOTS.md). |

[Русское руководство](../../../doc/MOVIECLIP_SOUND.ru.md).
## Converted timeline output / Преобразованный выход (v0.580)

```sh
/tmp/flashvm-build/flashvm_examples_multimedia_swf_event_mixer_main --convert-demo
/tmp/flashvm-build/flashvm_examples_multimedia_swf_event_mixer_main --convert-demo stereo.pcm
```

Реальный root callback остаётся тем же. Теперь source 44100 Hz mono подаётся в
явно настроенный 48000 Hz stereo mixer. Stdout: `first=-32768 tail=27208 active=0`.
Native S16 файл содержит восемь значений (четыре stereo frames):
-32768, -32768, 27208, 27208, 32512, 32512, 0, 0. Это raw PCM без WAV header.
Без пути файлы/устройства не открываются; с путём существующий файл заменяется.
Старые --demo/--timeline-demo сохраняют strict mono и прежние результаты.

[`FVM_SWFSoundMixerConfig`](../../../include/flashvm/swf_sound_mixer.h#L28) заполнен на стеке: точный struct_size, sample_rate,
channels, voice_capacity и flags. [`FVM_SWF_SOUND_MIXER_CONVERT`](../../../include/flashvm/swf_sound_mixer.h#L21) явно включает
интерполяцию/mono→stereo. Config копируется и не должен жить до close. Буферы
вмещают оба канала, а fwrite использует возвращённую геометрию. Чтение после
уничтожения source VM сохраняется, включая ещё слышимый buffered EOF.
Это линейный software resampler без anti-alias фильтра и без device clock;
не доказательство слышимого native вывода или качества original Flash Player.

| Additional API | Individual contract |
| --- | --- |
| [`fvm_swf_sound_mixer_create_configured`](../../../include/flashvm/swf_sound_mixer.h#L39) | Копирует явную политику, выделяет фиксированные slots/scratch, очищает out на отказе; source mismatch разрешён только флагом: [EN](../../../doc/SWF_SOUND_CONVERSION.md#fvm_swf_sound_mixer_create_configured) / [RU](../../../doc/SWF_SOUND_CONVERSION.ru.md#fvm_swf_sound_mixer_create_configured). |
| [`fvm_swf_sound_mixer_get_config`](../../../include/flashvm/swf_sound_mixer.h#L42) | Копирует immutable output geometry/flags без new, callbacks и продвижения; неверный struct_size сохраняет out: [EN](../../../doc/SWF_SOUND_CONVERSION.md#fvm_swf_sound_mixer_get_config) / [RU](../../../doc/SWF_SOUND_CONVERSION.ru.md#fvm_swf_sound_mixer_get_config). |

[Политика интерполяции и округления](../../../doc/SWF_SOUND_CONVERSION.ru.md#алгоритм-и-округление).
The [sound snapshot contract](../../../doc/SOUND_SNAPSHOT.md) / [контракт снимка](../../../doc/SOUND_SNAPSHOT.ru.md) describes bounded ordinal preparation. This tutorial uses only public callbacks and never mutates frame metadata.

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
| `fvm_gc_unpin` | function | [include/flashvm/gc.h:54](../../../include/flashvm/gc.h#L54) |
| `fvm_load_memory` | function | [include/flashvm/flashvm.h:72](../../../include/flashvm/flashvm.h#L72) |
| `fvm_movieclip_advance` | function | [include/flashvm/movieclip.h:99](../../../include/flashvm/movieclip.h#L99) |
| `fvm_movieclip_load_swf` | function | [include/flashvm/movieclip.h:76](../../../include/flashvm/movieclip.h#L76) |
| `fvm_movieclip_set_sound_callback` | function | [include/flashvm/movieclip.h:21](../../../include/flashvm/movieclip.h#L21) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../../../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../../../include/flashvm/flashvm.h#L66) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:20](../../../include/flashvm/standard.h#L20) |
| `fvm_swf_sound_mixer_close` | function | [include/flashvm/swf_sound_mixer.h:63](../../../include/flashvm/swf_sound_mixer.h#L63) |
| `FVM_SWF_SOUND_MIXER_CONVERT` | macro / define | [include/flashvm/swf_sound_mixer.h:21](../../../include/flashvm/swf_sound_mixer.h#L21) |
| `fvm_swf_sound_mixer_create` | function | [include/flashvm/swf_sound_mixer.h:45](../../../include/flashvm/swf_sound_mixer.h#L45) |
| `fvm_swf_sound_mixer_create_configured` | function | [include/flashvm/swf_sound_mixer.h:39](../../../include/flashvm/swf_sound_mixer.h#L39) |
| `fvm_swf_sound_mixer_get_config` | function | [include/flashvm/swf_sound_mixer.h:42](../../../include/flashvm/swf_sound_mixer.h#L42) |
| `fvm_swf_sound_mixer_info` | function | [include/flashvm/swf_sound_mixer.h:59](../../../include/flashvm/swf_sound_mixer.h#L59) |
| `fvm_swf_sound_mixer_read` | function | [include/flashvm/swf_sound_mixer.h:55](../../../include/flashvm/swf_sound_mixer.h#L55) |
| `fvm_swf_sound_mixer_stop_all` | function | [include/flashvm/swf_sound_mixer.h:61](../../../include/flashvm/swf_sound_mixer.h#L61) |
| `fvm_swf_sound_mixer_submit` | function | [include/flashvm/swf_sound_mixer.h:50](../../../include/flashvm/swf_sound_mixer.h#L50) |
| `FVM_SWFSoundMixerConfig` | struct | [include/flashvm/swf_sound_mixer.h:28](../../../include/flashvm/swf_sound_mixer.h#L28) |
<!-- source-links:end -->
