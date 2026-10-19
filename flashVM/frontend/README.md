# Desktop SWF frontend

[Русская версия](README.ru.md). This is an optional C89 desktop host for the VM,
not a dependency of the core library or compiler projects.

```sh
# Run from the flashVM component directory; install the SDL2 development SDK first.
cmake -S . -B build-desktop -DFVM_BUILD_DESKTOP_FRONTEND=ON
cmake --build build-desktop --target flashvm_player --parallel
./build-desktop/frontend/flashvm_player --help
./build-desktop/frontend/flashvm_player --jit --quality high --speed 150% movie.swf
./build-desktop/frontend/flashvm_player --no-jit --fps 30 first.swf second.swf
```

| Folder | Purpose |
| --- | --- |
| [linux](linux/README.md) | Linux main-thread launcher; SDL2 X11/Wayland presentation |
| [windows](windows/README.md) | Win32 UTF-16 command-line conversion and UTF-8 file paths |
| [macos](macos/README.md) | macOS main-thread launcher; SDL2 Cocoa presentation |
| [android](android/README.md) | SDLActivity test APK: three bundled movies, interpreter/JIT menu |
| [common](common/player.c) | Shared parser, VM lifecycle, timing and SDL window adapter |
| [tests](tests/smoke.py) | CLI/timing tests and opt-in native-display/presentation tests |

[Full options, API/source links, timing, ownership and limits](../doc/DESKTOP_FRONTEND.md).
Single files loop by default. Multiple files run sequentially for each authored
frame-count worth of ticks. Escape or window close exits the entire playlist.
The host advances/rasterizes the supported SWF subset. v0.583 adds HostInput
snapshots and real root PCM/ADPCM event output through optional SDL2 audio.
[Binary/test guide](../doc/PLAYER_TEST_BUILDS.md), [three SWFs](../examples/player/animation_audio/README.md).
Automatic A/V clock sync, full Flash Key/Mouse/button routing and browser integration
remain open. Windows/Android cross builds do not establish native runtime acceptance.

v0.584: [PCM/WAV диагностика и отправка ZIP с Android](../doc/PLAYER_AUDIO_DIAGNOSTICS.md).

v0.596: [Primary touch and shared key/pointer queue](../doc/PLAYER_POINTER.md)
([RU](../doc/PLAYER_POINTER.ru.md)); one touch contact needs no physical mouse.

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
