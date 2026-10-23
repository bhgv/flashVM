# Десктопное приложение для SWF — v0.577

Текущее gate/authored-Stop планирование: [контракт](MIXED_GATES.ru.md). Ограничения final-EOF-only/root-pause ниже относятся к исходному checkpoint v0.986 и заменены новым mixed scheduling.

Текущая root stream/event композиция и границы gate/pause/device: [контракт](DESKTOP_MIX.ru.md). Прежние versioned stream-only описания ниже исторические.
v0.819 проверяет CRC заголовка/side information MP3 и все статические корневые
MP3-ссылки до unmuted-аудиоустройства, один раз на ID. Stop-only ссылки и mute
не декодируют звук. [Допуск и ограничения](MP3_CRC.ru.md).


v0.780 добавляет поддержанный root MP3 stream/event decode с bounded native preparation и независимой проверкой PCM. Mixed root stream/event, negative latency и reference MP3 seek/A/V остаются открытыми. Free-format добавлен в v0.806–0.811; короткие события без достаточных заголовков остаются unsupported. [Контракт free-format](FREE_FORMAT_MP3.ru.md). [Текущий контракт](MP3_AUDIO.ru.md).

Обновление v0.582: [тестовые сборки, HostInput, звук и Android APK](PLAYER_TEST_BUILDS.ru.md).


[English](DESKTOP_FRONTEND.md).

## Сборка и запуск

Приложение целиком находится в [frontend/](../frontend/README.ru.md). В папках
Linux, Windows и macOS — точки входа, в common — общий C89-код. Это необязательная
часть компонента VM; отдельные компиляторы и линковщик от SDL2 не зависят.
Нужны toolchain C89/C++98, CMake и SDK разработки SDL2 версии 2.0.9 или новее с
CMake-пакетом и целью SDL2::SDL2. Команды из папки flashVM:

```sh
cmake -S . -B build-desktop -DFVM_BUILD_DESKTOP_FRONTEND=ON
cmake --build build-desktop --target flashvm_player --parallel
./build-desktop/frontend/flashvm_player --help
./build-desktop/frontend/flashvm_player --jit --quality best --speed 150% movie.swf
./build-desktop/frontend/flashvm_player --no-jit --fps 24 first.swf second.swf
```

В DEFS нужен GRAPHICS_AGG или GRAPHICS_CAIRO. HEADLESS в ядре означает вывод в
принадлежащий host RGBA-буфер: приложение показывает его через SDL2, не заменяя
растеризатор VM. Встроенные зависимости VM остаются статическими. SDL2 — внешний
SDK с собственными runtime-библиотеками и лицензией. Нестандартный SDK задаётся
через SDL2_DIR/CMAKE_PREFIX_PATH. При передаче изменений не предполагать, что
origin указывает на GitHub.

Подготовка ОС: [Linux](../frontend/linux/README.md),
[Windows](../frontend/windows/README.md), [macOS](../frontend/macos/README.md).
Для [Android](../frontend/android/README.md) с v0.582 есть отдельный SDLActivity/Surface APK target.

## Все параметры

Значения передаются отдельными аргументами, не в форме --option=value. Ключи
можно ставить до/после имён файлов до разделителя `--`. Пути с пробелами брать
в кавычки. После `--` допустимы пути, начинающиеся с дефиса. Поддержаны локальные
файлы, не URL.

| Ключ | Действие |
| --- | --- |
| `-h`, `--help` | Справка и успешный выход без открытия файла, display или VM |
| `--version` | Версия, ABI и метаданные сборки без открытия файлов и устройств |
| `--jit` | Автоматическая JIT-компиляция; отсутствие backend — ошибка до открытия окна. Неподдержанные тела могут исполняться интерпретатором |
| `--no-jit`, `--interpreter` | Принудительный интерпретатор, обход автоматического и прикреплённого JIT |
| `--quality low\|medium\|high\|best` | Начальное качество сцены, по умолчанию high; SWF впоследствии может его изменить |
| `--fps N` | Абсолютная частота тиков: десятичное число 0.1..1000, независимо от FPS файла |
| `--speed N` или `--speed N%` | 0.1..10000 процентов от FPS файла; по умолчанию 100 |
| `--once` | Ограничение числом тиков, равным объявленному числу кадров SWF |
| `--frames N` | Положительный целый предел тиков для каждого файла |
| `--finite-audio-tails` | Конечные event-only/root mixed хвосты с ожиданием текущих voice/retry/FIFO; [контракт](EVENT_TAILS.ru.md) |
| `--finite-mixed-tails` | Конечные root stream/event хвосты без нового padding; [контракт](FINITE_TAILS.ru.md) |
| `--mute` | Отключение событийного звука и проверенных корневых потоков без открытия устройства |
| `--stats` | Счётчики выполнения и раздельный mixed BASE/TAIL accepted/dequeued учёт; [контракт](MIX_ORIGIN_CLOCK.ru.md) |

Противоречащие JIT/interpreter, сочетания --fps с --speed и --frames с --once —
ошибка с кодом 2. Повтор одного ключа использует последнее значение. Отклоняются
неизвестные ключи, отсутствующие значения, NaN/Infinity, переполнение, нулевой
или знаковый предел кадров, пробелы внутри числа и неверные суффиксы. Знак %
разрешён только для --speed. Код 0 — справка/нормальное окончание/закрытие;
1 — ошибка загрузки, VM или окна; 2 — неправильные аргументы.

## Время и список файлов

Без явного режима host выбирает автоматический JIT, если backend доступен,
иначе интерпретатор. Только --stats подтверждает фактические native entries:
название запрошенного режима не доказывает исполнение всех тел машинным кодом.

FPS файла = frame_rate_fixed8 / 256. Итоговая частота — либо --fps, либо
FPS файла * проценты / 100. Эти режимы не перемножаются. Итог должен быть больше
нуля и не выше 1000. Очень медленные положительные частоты допустимы. При нулевом
FPS файла нужен явный --fps. Каждый тик вызывает [fvm_movieclip_advance](../include/flashvm/movieclip.h#L125) с одним
шагом, очищает фон, растеризует сцену и показывает результат. Первый тик входит
в кадр 1. Последний ограниченный кадр виден весь свой период. Expose/resize
перерисовывает сохранённую текстуру без нового тика VM, поэтому низкая скорость
не блокирует обновление окна или его закрытие.

Используются монотонные performance counters SDL и ожидание событий вместо
активного опроса. При большом отставании deadline сбрасывается без пропуска
VM-тиков. Нагрузка может снизить фактическую скорость; жёсткого real-time нет.
Изменяется частота timeline, не виртуальный GetTime, resampling аудио, декодирование
видео и не все таймеры Flash API.

Один файл по умолчанию работает до Escape/закрытия окна под управлением своего
скрипта. Несколько файлов запускаются по порядку с новыми VM/окном для каждого;
по умолчанию каждому выделяется объявленное число кадров в тиках. --frames
заменяет этот предел. stop/goto могут менять реально посещённые номера кадров:
считаются host-тики, а не уникальные кадры. Закрытие окна/Escape завершает весь
список. Ошибка любого файла прерывает список с ошибкой.

## Изображение и владение

Вход — файл с возможностью позиционирования, не более [FVM_SWF_DEFAULT_LIMIT](../include/flashvm/swf.h#L15)
(64 МиБ). Загрузка использует существующий профиль FWS/CWS. Импортёр сцены может
отклонить неподдержанные теги: приложение печатает статус VM, а не пропускает их.
Размер берётся из авторского RECT, не из границ видимых объектов. В пикселе
[FVM_SWF_TWIPS_PER_PIXEL](../include/flashvm/swf.h#L27) (20) twips. Размер округляется вверх и ограничен 1..4096
по каждой оси. Viewport вычитает левую/верхнюю границу сцены, не меняя _x/_y,
видимость, цвет или трансформации дочерних клипов.

Host владеет пикселями, окном, renderer/texture, context и runtime. Корневой клип
явно pin-ится до исполнения скриптов; GC step выполняется только после полного
render/present. Cleanup освобождает SDL-объекты, буферы, pin и VM при успехе и
ошибке. Адаптер окна не сохраняет VM-указателей. Все VM, SDL events и render
работают в главном потоке; фонового обращения к VM нет.

Каждый тик очищает сцену выбранным фоном или непрозрачным белым. SDL принимает
RGBA32, игнорирует alpha сцены при выводе в обычное OS-окно и сохраняет пропорции
с полями при resize. Пиксели заимствуются только на время upload. Качество
соответствует текущей [политике сцены](STAGE_QUALITY.md): LOW выключает vector AA;
полные различия medium/high/best, bitmap sampling и фильтры ещё требуют работы.

## Новые публичные функции VM

### fvm_swf_stage_bounds

Объявлена с [FVM_SWFStageBounds](../include/flashvm/swf.h#L29) в [swf.h](../include/flashvm/swf.h). Нужны owning
thread, загруженный context и выход с точным struct_size. OK копирует signed
x_min/x_max/y_min/y_max в twips. Равные границы допустимы как метаданные;
инвертированные дают INVALID_ARGUMENT. Этот же статус возвращается при null,
неверном размере, терминальном/collecting context или повреждённом canonical RECT.
UNSUPPORTED — context не загружен. Любая ошибка сохраняет весь выход. Аллокаций,
callbacks, скрипта, GC, borrowed-байтов и передачи владения нет.

### fvm_movieclip_render_view

Объявлена в [movieclip.h](../include/flashvm/movieclip.h). Аргументы: owning context,
его MovieClip, корректная host-owned RGBA-поверхность и ненулевой [FVM_Affine](../include/flashvm/path.h#L33) viewport.
Все шесть значений должны быть конечными. Сингулярная матрица сама по себе не
запрещена. Матрица копируется до getters и умножается слева на world-матрицу
каждого узла ровно один раз, включая детей. Обычные видимость/цвет корня и
скриптовые трансформации сохраняются.

Статусы: OK; INVALID_ARGUMENT для null/чужих/терминальных аргументов и NaN/Inf;
OUT_OF_MEMORY при отказе native-аллокации; прежние render/getter/script-статусы,
включая UNSUPPORTED/EXCEPTION. Неверный viewport отклоняется до рисования; более
поздняя ошибка может оставить частично нарисованный буфер, как обычный render.
Транзакционности нет. Caller обязан root-ить клип, удерживать буфер до возврата
и не перекрывать viewport/source/output storage. Указатель viewport не сохраняется.
Native guard блокирует collection/release во время callbacks. Вызов — только
в owning thread вне терминальных фаз GC.

## Исходники и внутренние функции

| Файл | Функции и ответственность |
| --- | --- |
| [options.c](../frontend/common/options.c) | [`decimal`](../frontend/common/options.c#L11)/frame_limit проверяют числа; [`fvm_desktop_parse`](../frontend/common/options.h#L58) публикует параметры атомарно, используя caller-owned scratch array путей; [`fvm_desktop_rate`](../frontend/common/options.h#L61) проверяет итоговую частоту; [`fvm_desktop_help`](../frontend/common/options.h#L64) выводит справку |
| [player.c](../frontend/common/player.c) | [`read_movie`](../frontend/common/player.c#L60) читает ограниченный UTF-8 файл; [`checked`](../frontend/common/player.c#L111) сообщает статус операции; [`clear_stage`](../frontend/common/player.c#L120) очищает фон; [`play`](../frontend/common/player.c#L211) владеет VM/окном и deadline; [`fvm_desktop_main`](../frontend/common/player.h#L61) разбирает CLI и список файлов |
| [sdl_window.c](../frontend/common/sdl_window.c) | [`fvm_desktop_window_open`](../frontend/common/window.h#L53) / [`fvm_desktop_window_present`](../frontend/common/window.h#L56) / [`fvm_desktop_window_events`](../frontend/common/window.h#L59) / [`fvm_desktop_window_close`](../frontend/common/window.h#L88) — граница ресурсов; [`draw_texture`](../frontend/common/sdl_window.c#L109)/[`handle_event`](../frontend/common/sdl_window.c#L331) — повторный показ и закрытие; [`fvm_desktop_now`](../frontend/common/window.h#L83) — монотонное время; [`fvm_desktop_window_error`](../frontend/common/window.h#L85) — borrowed сообщение |
| [options.h](../frontend/common/options.h), [window.h](../frontend/common/window.h) | Внутренние структуры/константы и контракты, не стабильный публичный ABI VM |

## Проверки и незавершённое

На Linux проверяются настоящие SDL2/X11 software-окна в Xvfb, FWS/CWS, отрицательный
origin, пути с кириллицей/пробелами, обе политики времени, список файлов, реальные
счётчики interpreter/JIT, readback цветов и закрытие. Core-тесты проверяют signed
RECT разных разрядностей, неизменность ошибочного выхода, проекцию/видимость без
изменения скриптовой геометрии. Справка и неверные CLI проверяются с заведомо
несуществующим video driver: display не инициализируется.

Оконные проверки включаются явно FVM_DESKTOP_WINDOW_TESTS=ON и запускаются через
CTest в сеансе display. Для cross-build они не регистрируются. Это не нативная
приёмка Windows/macOS, Wayland, физического GPU или Android. v0.582 добавляет root event audio и HostInput, но полной A/V синхронизации,
стандартных Flash keyboard/button handlers, browser plugin и file association пока нет
и .app-пакета. Это приложение для реализованного подмножества VM, а не заявление
о полном Flash Player или завершении стадии 9.
Проверка Linux v0.577: 1140 успешных тестов, 3 явных пропуска нативных
тестов других ОС из 1143 зарегистрированных (examples выключены).
Дополнительно прошли 4 проверки ядра с Cairo и 8 тестов просмотрщика.


Текущая ограниченная рассылка Key/Mouse и адаптер первичного касания описаны
во [вводе плеера](PLAYER_POINTER.ru.md). Полная scene/button рассылка ещё открыта.
[Текущие метаданные сборки](PLAYER_BUILD_INFO.ru.md) · [GC и аргументы C-хоста](PLAYER_EVENT_GC.ru.md).
[Structured C89/C++98 host / Встраивание через структуру](PLAYER_CONFIGURATION.ru.md).
[Memory, HTTP(S), SWF root loading and library modules / Источники и модули](PLAYER_SOURCES.ru.md).

[Owner playback commands / Управление воспроизведением](PLAYBACK_CONTROL.md) / [RU](PLAYBACK_CONTROL.ru.md).


[Playback mailbox / Ящик команд](PLAYBACK_MAILBOX.ru.md).


[Logical RGBA frames / RGBA-кадры сцены](FRAME_CAPTURE.ru.md).
Windowless logical-stage adapter / Адаптер без окна: [EN](OFFSCREEN_PLAYER.md) / [RU](OFFSCREEN_PLAYER.ru.md).
Queued resource owner control / Очередь управления ресурсами: [EN](RESOURCE_DISPATCH.md) / [RU](RESOURCE_DISPATCH.ru.md).


Desktop stream preparation now uses [`fvm_swf_stream_sound_snapshot_open_pcm_inferred`](../include/flashvm/swf_stream_sound.h#L137): canonical whole ADPCM packets remain accepted; partial packets require a unique byte/padding endpoint. Ambiguous counts stay UNSUPPORTED before audio acquisition. Aggregate retained/peak budgets still apply. / Подготовка коротких ADPCM блоков разрешена только при единственной длине; неоднозначные блоки не угадываются. [EN](INFERRED_ADPCM.md) / [RU](INFERRED_ADPCM.ru.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Affine` | struct | [include/flashvm/path.h:33](../include/flashvm/path.h#L33) |
| `fvm_movieclip_advance` | function | [include/flashvm/movieclip.h:125](../include/flashvm/movieclip.h#L125) |
| `fvm_movieclip_render_view` | function | [include/flashvm/movieclip.h:138](../include/flashvm/movieclip.h#L138) |
| `FVM_SWF_DEFAULT_LIMIT` | macro / define | [include/flashvm/swf.h:15](../include/flashvm/swf.h#L15) |
| `fvm_swf_stage_bounds` | function | [include/flashvm/swf.h:38](../include/flashvm/swf.h#L38) |
| `fvm_swf_stream_sound_snapshot_open_pcm_inferred` | function | [include/flashvm/swf_stream_sound.h:137](../include/flashvm/swf_stream_sound.h#L137) |
| `FVM_SWF_TWIPS_PER_PIXEL` | macro / define | [include/flashvm/swf.h:27](../include/flashvm/swf.h#L27) |
| `FVM_SWFStageBounds` | struct | [include/flashvm/swf.h:29](../include/flashvm/swf.h#L29) |
<!-- source-links:end -->
