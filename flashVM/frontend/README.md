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

## Retained ClipActions v0.625

PlaceObject2 sprite records are retained; the v0.625 baseline delivered only
EnterFrame during ticks. Since v0.626 global scene input is wired as below.
Load/unload/construct/data and targeted interaction still require explicit host integration through [the C89 API](../doc/CLIP_ACTIONS.md).
No native onX method, hit testing or lifecycle scheduler is implicitly added.
The optional flashvm_player_clip_actions_interpreter/jit tests execute SWF5–8 on
actual SDL dummy video and reject wrong-value controls; this is not visible-device
or foreign-platform acceptance.

## Retained scene input v0.626

The main player now supplies its pinned root to fvm_desktop_window_pump_clip_input:
namespace/focus handlers and native state precede each global retained scene event.
The namespace-only wrapper remains available to existing hosts. Primary touch keeps
its existing mapping. [Exact contracts, errors and remaining limits](../doc/CLIP_INPUT.md).
The flashvm_frontend_clip_input test uses real SDL dummy video with synthetic
mouse/finger/key messages. The separate flashvm_frontend_clip_player executable uses
the actual host/adapter and injects SDL queue messages after its first presentation;
flashvm_player_clip_input_interpreter/jit verify SWF5–8 event sums and coalesced redraw.
Only this test executable injects events; no physical-device acceptance is implied.

## MovieClip input methods v0.627

The combined scene route now delivers records followed by optional onMouseMove/Down/Up
and onKeyDown/Up for SWF6–8; SWF5 keeps records only. The namespace-only pump and
retained-only API remain. State/namespace precede scene delivery; observer-only
keyboard publication avoids a duplicate focused-clip call. Rootless hosts keep
focus-first delivery. A method
failure consumes its transition, leaving later ones queued. [Contract](../doc/CLIP_INPUT_METHODS.md).
flashvm_player_clip_input_methods_interpreter/jit use the actual SDL queue and host:
method-only and record → method traces, source gates, wrong-value controls and
method-originated redraw. Dummy video/synthetic messages are not physical/foreign
platform acceptance.

## ClipEventKeyPress v0.628

The player now translates the original queued key-down snapshot after successful
scene KeyDown records/methods and broadcasts retained KeyPress. Special SWF selectors
differ from virtual Key codes; Shift/Caps ASCII comes from the existing English mapper.
Release/unmapped messages skip KeyPress; autorepeat delivers it. Epoch reset and
earlier errors suppress the stage; a KeyPress error leaves later FIFO transitions.
Escape/Android back keep their close policy. [Contract](../doc/CLIP_KEYPRESS.md).
New flashvm_frontend_clip_keypress and flashvm_player_clip_keypress_interpreter/jit
checks use actual SDL adapter/host with synthetic input, not physical-device evidence.

[Presentation recovery](../doc/PLAYER_RECOVERY.md).

[Wheel and cursor bridge](../doc/PLAYER_WHEEL_CURSOR.md).

[Bounded event processing](../doc/PLAYER_EVENT_FAIRNESS.md).

[Compatibility and input preflight](../doc/PLAYER_PREFLIGHT.md).

[Collection between frames](../doc/PLAYER_EVENT_GC.md).

[Build metadata and C host interface](../doc/PLAYER_BUILD_INFO.md).

## Whole-packet ADPCM root streams (v0.764)

[Player contract and bounded decoder](../doc/PLAYER_PCM_STREAMS.md); [runnable ADPCM example](../examples/player/pcm_stream/README.md#whole-packet-adpcm-variant-v0764). No FFmpeg runtime dependency; other codecs/partial packets and root event mixing remain unsupported.

## Aggregate stream budget (v0.767)

[64-MiB prepared-source policy](../doc/PLAYER_STREAM_BUDGETS.md) includes retained snapshots, the segment table and ADPCM expansion peaks. Rejection occurs before native startup; mute prepares no snapshots.

## Stream recovery v0.769 / Восстановление очереди

Observed empty native stream queues with authorized work are paused, primed and
resumed by the worker. Initial/gated/finished streams are excluded. Existing
audio_underruns and optional bounded traces report observations, not DAC behavior.
Пустая очередь при разрешённых данных вызывает повторное наполнение; начало,
будущие кадры и EOF не считаются underrun. Ошибки backend передаются владельцу.
[EN](../doc/PLAYER_STREAM_RECOVERY.md) / [RU](../doc/PLAYER_STREAM_RECOVERY.ru.md).

Windows x64 explicit Zig/static-SDL build instructions (v0.770): [EN](../doc/CROSS_WINDOWS_ZIG.md) / [RU](../doc/CROSS_WINDOWS_ZIG.ru.md). Build/link evidence remains separate from Windows execution and device acceptance. Кросс-сборка не заменяет запуск на Windows.
