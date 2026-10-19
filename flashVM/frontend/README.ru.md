# Десктопный проигрыватель SWF

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
