# Player test builds — v0.582

v0.616: the common player explicitly installs AsBroadcaster for SWF6–8 before
scene execution; older movies skip it. [Contract EN](AS_BROADCASTER.md#transport-and-player-v0616) /
[контракт RU](AS_BROADCASTER.ru.md#перенос-и-плеер-v0616). Native target rebuilds
and physical playback acceptance are separate from the Linux software CLI fixtures.


[Русское руководство](PLAYER_TEST_BUILDS.ru.md).
This checkpoint delivers Linux x64/Windows x64 binary packages and a separately
packaged Android ARM64 test APK, plus [two runnable SWF8 movies](../examples/player/animation_audio/README.md).
They exercise the implemented subset, not a full Flash Player or stage-9 release.

## Packages and controls

Linux: unpack, chmod +x flashvm_player/run-interpreter.sh/run-jit.sh and execute
one launcher from a normal desktop session. The launcher changes to its own
folder, selects --no-jit or --jit, enables --stats, and defaults to the interactive
movie; additional arguments select another SWF. A lib/ directory contains SDL2.
Built on Ubuntu 24.04 x86-64 (glibc 2.39); GCC runtimes are static. SDL's system
window/audio dependencies still need to be installed. This is not a universal
binary for older glibc distributions. Missing audio endpoint is an error; use --mute.

Windows: keep flashvm_player.exe, SDL2.dll and libwinpthread-1.dll together.
Double-click run-interpreter.bat/run-jit.bat, or use the console commands in the
[test README](../examples/player/animation_audio/README.md#launch--запуск).
These are PE32+ x86-64 console applications with a real SDL window. Unicode paths
use the existing UTF-16 command-line adapter. Cross-built using MinGW GCC 13 POSIX;
actual Windows execution remains unverified here. DLL imports were inspected.

Android: install the test-signed APK (Android 6/API23+, arm64-v8a), enable install
permission for the application opening the downloaded APK when Android asks,
then choose a movie and Interpreter/JIT in the menu. A Mute checkbox skips output
opening. Back exits the movie process to the menu. The APK contains both movies,
SDL Java/native support and a private-file asset loader, not an arbitrary SWF
file browser/SAF importer. No storage/microphone/network permissions are requested.
Physical keyboard arrows/Space and primary touch/mouse are supported. No soft
keyboard, multitouch recognizer or arbitrary intents in this initial test package.

APK native libraries use 16 KiB-aligned PT_LOAD segments; the package is zipaligned
for uncompressed native libraries and verified with apksigner. Disposable test
signature, not a production signing identity; upgrading a differently signed test
may require uninstalling it. Build/signature/ABI checks do not establish runtime
acceptance on a phone. No emulator/device run was available in this checkpoint.

Escape/window close exits the desktop playlist. A single file loops; --once or
--frames bounds it. Resize preserves logical stage aspect. Use --help for all
options, including --quality/--fps/--speed/--mute and interpreter/JIT selection.
These movies have six root sound controls per five-second loop. Quiet U8 mono,
S16 stereo and ADPCM tones are supplied, with no external copyrighted media.

## HostInput extension

The C89 host [player.c](../frontend/common/player.c) creates a pinned object bound
as global HostInput and copies [SDL state](../frontend/common/sdl_window.c) into
its scalar fields immediately before each [`fvm_movieclip_advance`](../include/flashvm/movieclip.h#L62). All VM/input/
render calls stay serialized on the SDL application thread. Managed root/input
objects are pinned, then unpinned before context destruction. Native event queues
and PCM ownership remain outside GC.

| Field | Units and behavior |
| --- | --- |
| x, y | Latest primary pointer in authored world/stage pixels, including stage-origin restoration. SDL maps window events through the logical viewport; letterbox positions may be outside the authored stage. Also supplied to [`fvm_input_set_pointer`](../include/flashvm/input.h#L72) for ordinary _xmouse/_ymouse semantics. |
| left, right, up, down, space | Held arrow/Space states, numeric 0/1. |
| buttons | Held primary/middle/secondary pointer bits 1/2/4. No exclusive global OS capture. |
| keyCode | Latest transition code: uppercase ASCII letters, digits/control ASCII, arrows 37..40; unsupported non-ASCII keys give zero. |
| keyDown | Latest key transition's 0/1 state, not a complete arbitrary-key held map. |
| keyPresses, clicks | Cumulative physical down-transition counters; repeat keydowns do not increase keyPresses. Unsigned host counters can wrap; not persistent event history. |

Focus loss clears held state, preserving counters/position/code. Android background
pauses timeline and FIFO, resets held state, and rebases the wall-clock deadline
on return; it does not run a catch-up burst. SDLActivity owns Activity/Surface/audio
thread coordination. HostInput is a player extension, not the standard Flash Key/
Mouse broadcaster API. Arbitrary SWFs expecting those interfaces are not promised
compatible. No user event handlers are dispatched by this snapshot mechanism.
The API is per-frame state: a down/up between frames may only change counters.

## Audio ownership and scheduling (v0.582; see v0.583 correction below)

[audio.c](../frontend/common/audio.c) queries [`fvm_swf_sound_event_count`](../include/flashvm/swf_sound.h#L56). With root
controls and no --mute it opens a real 48000 Hz stereo SDL device, a 32-voice
configured mixer and a 512-frame producer with a 2048-frame FIFO target. Registers
[`fvm_movieclip_set_sound_callback`](../include/flashvm/movieclip.h#L21); each actual entry submits its ordinal to
[`fvm_swf_sound_mixer_submit`](../include/flashvm/swf_sound_mixer.h#L50). Decode/conversion errors fail visibly.
At most four [`fvm_swf_sound_output_pump`](../include/flashvm/swf_sound_output.h#L55) calls per loop visit; waits are capped at five milliseconds
while output exists. Queue copying and playback run through the real
[SDL bridge](SWF_SOUND_OUTPUT.md). Producer work is not realtime callback work.

Native PCM/ADPCM event voices, loops, no-multiple and stop-by-resource are exercised.
The movie wall clock and output device clock are independent: --fps/--speed changes
entry intervals, not sound pitch. There is queue/hardware latency; no drift-corrected
A/V sync is claimed. MP3/AAC/streams/sprite sound/AS Sound and AVM1 StopSounds routing
to this separate player mixer remain open. Existing context-channel StopSounds API
is unchanged. Queue stop/close precedes mixer/device close and SDL_Quit, including
partial initialization/error cleanup. Unregister callbacks before releasing userdata.

## Reproduce desktop builds

From the repository root, using the explicit lean AGG/JIT profile (no fonts/image
codecs/FFmpeg/SQLite SDKs):

```sh
cmake -S flashVM -B build-player-linux -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs" -DFVM_BUILD_DESKTOP_FRONTEND=ON -DFVM_ENABLE_SDL_AUDIO=ON -DFVM_BUILD_TESTS=OFF -DFVM_BUILD_EXAMPLES=OFF -DCMAKE_BUILD_TYPE=Release "-DCMAKE_EXE_LINKER_FLAGS=-static-libgcc -static-libstdc++"
cmake --build build-player-linux --target flashvm_player --parallel
```

Requires installed SDL2 >=2.0.9 CONFIG/SDL2::SDL2 and a C89/C++98 toolchain. Set
SDL2_DIR if needed. The Linux executable loads a packaged SDL2 from $ORIGIN/lib,
falling back to normal system lookup; keep its versioned SONAME filename.
For a detached VM component replace -S flashVM with -S . and use that component's
absolute profile path; there are no compiler/linker sibling dependencies.

Linux-to-Windows cross build (installed MinGW POSIX compiler on PATH, SDL2 MinGW SDK):

```sh
cmake -S flashVM -B build-player-win -DCMAKE_TOOLCHAIN_FILE="$PWD/flashVM/cmake/toolchains/mingw-x64.cmake" -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs" -DSDL2_DIR=/path/to/SDL2/x86_64-w64-mingw32/lib/cmake/SDL2 -DFVM_BUILD_DESKTOP_FRONTEND=ON -DFVM_ENABLE_SDL_AUDIO=ON -DFVM_BUILD_TESTS=OFF -DFVM_BUILD_EXAMPLES=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-player-win --target flashvm_player --parallel
```

Keep matching SDL2.dll and the compiler's libwinpthread-1.dll. zlib uses its
supported 32-bit CRC braid on LLP64 Windows to preserve strict C89 without
editing upstream sources. Windows speech includes base Windows types before SAPI.
Native Windows/MSVC instructions remain in [frontend/windows](../frontend/windows/README.md).
Android SDK/NDK packaging: [separate reproducible guide](../frontend/android/README.md).

## Checks and limits

The default Linux Release core suite and explicit X11 frontend suite are separate.
FVM_DESKTOP_WINDOW_TESTS enables geometry/CLI/pixel/close checks.
FVM_DESKTOP_DEMO_TESTS additionally requires Linux X11, SDL audio, Python3/Pillow
and xdotool, with an existing DISPLAY (Xvfb is acceptable but must be identified).
The real test injects arrow/mouse/Space, inspects resulting colors/positions and
checks actual native/interpreter and sound-event counters. Audio uses dummy here:
protocol success is not audible hardware acceptance. Tests run both demo files in
both modes; no foreign-platform execution is inferred from cross compilation.
[Current evidence](ROADMAP_STATUS.md). All roadmap stages remain open.

## v0.583 Android audio correction

The v0.582 Android user report describes sporadic knocks/crackling. Its render-loop
producer and fixed 2048-frame (about 43 ms) reserve could underrun during rendering
or when Android supplied a larger device quantum. This is a plausible cause,
not a device-confirmed diagnosis.

The current player uses a native SDL worker, 1024-frame pump blocks and a target
of max(4800 frames, four obtained device blocks). Resume requires at least two
device blocks or 1024 frames. The worker refills every 5 ms, at most eight pumps
per pass. Source submission, pause, stop and queue inspection share a mutex;
VM/GC inspection stays on the owning main thread. This increases buffering
latency to keep rendering stalls from interrupting PCM. It is not hard realtime
or drift-corrected A/V synchronization. `--stats` adds audio_underruns and
sound_stops; SDL logging includes actual device quantum/target/resume geometry.

StopSounds and standard stopAllSounds now stop this prepared mixer/queue through
[context bindings](AUDIO_STOP_BINDINGS.md), even before first PCM submission.
[03-stop-sounds.swf](../examples/player/animation_audio/03-stop-sounds.swf) cancels
the initial tone and loop, then plays the later stereo and ADPCM tones.
The menu includes three movies × two execution modes. Existing v0.582 Linux and
Windows downloads retain their old implementation; this change requires rebuilding.
The new Android APK needs listening on the affected phone in both modes and
foreground/background transitions. Linux dummy-driver checks cannot prove that.

## v0.592 Selection test / Проверка Selection

SWF 02 now sets root focusEnabled and calls Selection.setFocus("_root") once.
Root-focused press/release counters are independent of the Key listener; each
focused release increases the blue box height. The root remains selected across
loops. The player explicitly installs the MovieClip-only namespace for SWF6–8;
automatic mouse/Tab focus and Android virtual-keyboard/IME input remain open.
[EN contract](SELECTION.md) / [RU](SELECTION.ru.md).

В SWF 02 отдельные обработчики выбранного корня и слушателя Key считают одно
нажатие/отпускание независимо. Высота синего квадрата показывает отпускания корню.
Установка Selection явная и только для SWF6–8. Это не автоматический выбор
мышью/Tab и не реализация экранной IME-клавиатуры Android. Исходные звуковые
данные и кодирование тонов сохранены.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_input_set_pointer` | function | [include/flashvm/input.h:72](../include/flashvm/input.h#L72) |
| `fvm_movieclip_advance` | function | [include/flashvm/movieclip.h:62](../include/flashvm/movieclip.h#L62) |
| `fvm_movieclip_set_sound_callback` | function | [include/flashvm/movieclip.h:21](../include/flashvm/movieclip.h#L21) |
| `fvm_swf_sound_event_count` | function | [include/flashvm/swf_sound.h:56](../include/flashvm/swf_sound.h#L56) |
| `fvm_swf_sound_mixer_submit` | function | [include/flashvm/swf_sound_mixer.h:50](../include/flashvm/swf_sound_mixer.h#L50) |
| `fvm_swf_sound_output_pump` | function | [include/flashvm/swf_sound_output.h:55](../include/flashvm/swf_sound_output.h#L55) |
<!-- source-links:end -->
