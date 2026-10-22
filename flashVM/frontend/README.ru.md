# Десктопный проигрыватель SWF

Mixed-события продолжаются при future stream gates и authored root Stop; host pause удерживает всю очередь. [Текущий контракт](../doc/MIXED_GATES.ru.md).

Root stream и StartSound используют общую tracked-очередь: [контракт и ограничения](../doc/DESKTOP_MIX.ru.md).
v0.819 проверяет CRC заголовка/side information MP3 и все статические корневые
MP3-ссылки до unmuted-аудиоустройства, один раз на ID. Stop-only ссылки и mute
не декодируют звук. [Допуск и ограничения](../doc/MP3_CRC.ru.md).


Desktop SyncNoMultiple учитывает ID до конца очереди, включая короткие голоса, уже освобождённые микшером. Таблица ID выхода учитывается отдельно от бюджета источников. [Контракт очереди/управления](../doc/QUEUED_SOUND_IDS.ru.md).

Подготовка событийных источников выполняется вне mutex аудиопотока; отдельный бюджет микшера/голосов по умолчанию — 64 MiB. VM/очереди/trace учитываются отдельно. [Владение и ограничения](../doc/NATIVE_VOICES.ru.md).

v0.780 добавляет поддержанный root MP3 stream/event decode с bounded native preparation и независимой проверкой PCM. Mixed root stream/event, negative latency и reference MP3 seek/A/V остаются открытыми. Free-format добавлен в v0.806–0.811; короткие события без достаточных заголовков остаются unsupported. [Контракт free-format](../doc/FREE_FORMAT_MP3.ru.md). [Текущий контракт](../doc/MP3_AUDIO.ru.md).

[English](README.md). Необязательное C89-приложение поверх VM. Ядро и отдельные
проекты компиляторов не зависят от SDL2 и этой точки запуска.

```sh
# Из папки компонента flashVM; предварительно установить SDK разработки SDL2.
cmake -S . -B build-desktop -DFVM_BUILD_DESKTOP_FRONTEND=ON
cmake --build build-desktop --target flashvm_player --parallel
./build-desktop/frontend/flashvm_player --help
./build-desktop/frontend/flashvm_player --jit --quality high --speed 150% movie.swf
./build-desktop/frontend/flashvm_player --no-jit --fps 30 first.swf second.swf
```

| Папка | Назначение |
| --- | --- |
| [linux](linux/README.md) | Главный поток Linux; SDL2-окно X11/Wayland |
| [windows](windows/README.md) | Win32: аргументы UTF-16 преобразуются в UTF-8 |
| [macos](macos/README.md) | Главный поток macOS; окно Cocoa через SDL2 |
| [android](android/README.md) | SDLActivity APK: три SWF и меню interpreter/JIT |
| [common](common/player.c) | Общие параметры, жизненный цикл VM, таймер и вывод SDL2 |
| [tests](tests/smoke.py) | CLI/таймер и явно включаемые тесты настоящего окна |

[Полное руководство: ключи, API, исходники и ограничения](../doc/DESKTOP_FRONTEND.ru.md).
Один файл по умолчанию зациклен. Несколько файлов запускаются последовательно:
каждому отводится число тиков, указанное как число кадров SWF. Escape/закрытие
окна завершает весь список. v0.583 добавляет HostInput snapshots и настоящий
root PCM/ADPCM event output через опциональный SDL2 audio.
[Пакеты и тесты](../doc/PLAYER_TEST_BUILDS.ru.md), [три SWF](../examples/player/animation_audio/README.md).
Автоматический A/V sync, полные Flash Key/Mouse/button handlers и browser остаются
открытыми. Windows/Android cross builds не заменяют нативную runtime приёмку.

v0.584: [PCM/WAV диагностика и отправка ZIP с Android](../doc/PLAYER_AUDIO_DIAGNOSTICS.ru.md).

v0.596: [Первичное касание и общая очередь клавиш/указателя](../doc/PLAYER_POINTER.ru.md)
([EN](../doc/PLAYER_POINTER.md)); физическая мышь для касания не нужна.

## Timer wakeups / Пробуждение таймеров (v0.622)

The active loop delivers scheduled callbacks between timeline frames and caps
SDL waits with fvm_timers_next_delay. Input runs before timer polling; safe GC
steps also reclaim callback activations between slow frames. Rendering keeps its
frame cadence. Background/final-frame waits retain their existing suspension.

Активный цикл доставляет таймеры между кадрами и ограничивает ожидание SDL
ближайшим deadline. Ввод обрабатывается до poll; callback-активации могут
собираться между редкими кадрами. Rendering остаётся по частоте фильма.

Contracts / Контракты: [EN](../doc/TIMER_WAKEUP.md) / [RU](../doc/TIMER_WAKEUP.ru.md).
Actual SDL dummy-video regression (no visible-window/device acceptance):

```sh
ctest --test-dir build -R '^flashvm_player_timers_' --output-on-failure
```

SWF6–8, interpreter/native JIT, 10 ms interval at 2 fps; a second-frame script
checks multiple deliveries, and a damaged count fixture must fail.

## updateAfterEvent (v0.623)

Eligible interval/Mouse/focused-clip callbacks can request a coalesced redraw
between frames. The player presents the current scene without advancing timeline;
`--stats` reports `presentations` and interframe `refreshes` separately from `frames`.
Requests made during drawing remain pending. Background/final-frame hold policy
is unchanged. Contracts: [EN](../doc/UPDATE_AFTER_EVENT.md) / [RU](../doc/UPDATE_AFTER_EVENT.ru.md).

Допустимые interval/Mouse/focused-clip callback запрашивают объединяемый redraw
между кадрами без продвижения timeline. Счётчики presentations/refreshes отделены
от frames. Запросы во время рисования не теряются; политика фона и финального
ожидания прежняя. Тесты разделяют настоящий SDL dummy-video и протокольный capture
с реальными raster-пикселями; это не приёмка видимого окна/устройства.

```sh
ctest --test-dir build -R '^flashvm_(player|capture)_update_' --output-on-failure
```

## Сохраняемые ClipActions v0.625

PlaceObject2 сохраняет records sprite; базовая v0.625 доставляла только
EnterFrame во время ticks. С v0.626 global input подключён, как описано ниже.
Load/unload/construct/data и targeted interaction требуют явной host
интеграции [C89 API](../doc/CLIP_ACTIONS.ru.md). Native onX, hit testing и lifecycle
scheduler автоматически не добавлены. Optional tests flashvm_player_clip_actions_
interpreter/jit исполняют SWF5–8 на настоящем SDL dummy video и отвергают wrong-value
controls; это не видимый экран или приёмка другой ОС.

## Input дерева клипов v0.626

Основной player передаёт pinned root в fvm_desktop_window_pump_clip_input: namespace/
focus handlers и native state предшествуют каждому global retained scene event.
Старый wrapper остаётся namespace-only, primary touch mapping сохраняется.
[Контракты/ошибки/границы](../doc/CLIP_INPUT.ru.md). flashvm_frontend_clip_input
использует реальную SDL dummy video с synthetic mouse/finger/key messages. Отдельный
flashvm_frontend_clip_player использует настоящий host/adapter, добавляя SDL messages
после первого показа; flashvm_player_clip_input_interpreter/jit проверяют SWF5–8,
сумму событий и coalesced redraw. Injector только тестовый; физическое устройство
не считается проверенным.

## MovieClip input methods v0.627

Общий scene route теперь вызывает записи и затем optional onMouseMove/Down/Up,
onKeyDown/Up для SWF6–8 (SWF5 получает только записи). Старый namespace-only pump и
retained-only API сохранены. State/namespace остаются перед scene delivery;
observer-only публикация клавиатуры предотвращает дубль focused clip. Pump без root
сохраняет focus-first поведение;
ошибка метода потребляет текущий переход, не следующие. [Контракт](../doc/CLIP_INPUT_METHODS.ru.md).
flashvm_player_clip_input_methods_interpreter/jit используют реальный SDL queue/host:
method-only и record → method trace, source gate, ошибочные контроли и redraw из методов.
Dummy video и synthetic messages не заменяют физическую/межплатформенную приёмку.

## ClipEventKeyPress v0.628

Плеер теперь переводит исходный queued key-down snapshot после успешных scene
KeyDown records/methods и рассылает retained KeyPress. Special SWF-коды отличны от
Key virtual codes; Shift/Caps ASCII берётся из прежнего English mapper. Release/
unmapped не рассылаются, autorepeat рассылается. Epoch reset и ошибки ранних стадий
подавляют KeyPress; ошибка самого KeyPress оставляет следующие FIFO transitions.
Escape/Android back остаются зарезервированы для закрытия. [Контракт](../doc/CLIP_KEYPRESS.ru.md).
Новые тесты flashvm_frontend_clip_keypress и flashvm_player_clip_keypress_interpreter/jit
проверяют настоящий SDL adapter/host с synthetic input, не физическое устройство.

[Восстановление изображения](../doc/PLAYER_RECOVERY.ru.md).

[Колесо и курсор](../doc/PLAYER_WHEEL_CURSOR.ru.md).

[Ограниченная обработка событий](../doc/PLAYER_EVENT_FAIRNESS.ru.md).

[Совместимость и проверка входа](../doc/PLAYER_PREFLIGHT.ru.md).

[Сборка мусора между кадрами](../doc/PLAYER_EVENT_GC.ru.md).

[Метаданные сборки и C-интерфейс](../doc/PLAYER_BUILD_INFO.ru.md).

## Корневые потоки из целых ADPCM-пакетов (v0.764)

[Контракт плеера и декодера](../doc/PLAYER_PCM_STREAMS.ru.md), [запускаемый пример ADPCM](../examples/player/pcm_stream/README.ru.md). FFmpeg для воспроизведения не нужен; другие кодеки, короткие пакеты и смешивание с корневыми событиями пока не поддержаны.

## Общий бюджет потоков (v0.767)

[Бюджет подготовленных источников 64 МиБ](../doc/PLAYER_STREAM_BUDGETS.ru.md) включает снимки, таблицу и временный пик ADPCM. Превышение отклоняется до native startup; mute не подготавливает снимков.
