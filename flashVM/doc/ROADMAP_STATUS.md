# FlashVM stage status / Состояние стадий — v0.585

This is a bounded review of existing implementation reports and the current
explicit rate/channel conversion work. It does not replace a complete per-API/SWF conformance matrix
or certify unexecuted platforms. All nine stages remain open. Existing test
counts are historical unless explicitly rerun for this revision.

Это сводка существующих отчётов и текущей конвертации частоты/каналов, а не
полный повторный аудит всех API. Все девять стадий открыты. Старые результаты
тестов не выдаются за новый прогон; проценты и сроки из них не выводятся.

| Stage / Этап | Existing work / Основа | Remaining acceptance / Что не закрыто |
| --- | --- | --- |
| 1: objects/types | Object/Array/String, property cases and ownership reports in [COMPATIBILITY_STATUS.md](COMPATIBILITY_STATUS.md) | Complete version-specific object/string semantics and reference comparisons; old roadmap entries do not imply every listed feature is wholly absent |
| 2: AVM1 | All 100 bounded dispatch-catalog entries have source references; [AVM1_DISPATCH_INVENTORY.md](AVM1_DISPATCH_INVENTORY.md) | Full operand, scope, exception, conversion and version semantics against original behavior; source branches alone are insufficient |
| 3: scene/timeline | MovieClip/resource/control/geometry reports; root sound-block framing | Full scene/resource behavior, automatic audio clock, sprite sound integration and real movie/reference coverage |
| 4: graphics/media | Vector/text adapters, explicit audio voices/mixing, PCM streams, native multimedia bridges | Complete effects/text/codecs/video and device playback/capture. v0.576 adds encoded capture and MP3-prefix inspection, not MPEG decoding |
| 5: standard APIs | Implemented standard objects and bridge contracts in the compatibility register | Complete method/object inventory and missing networking, loading, XML, event/input/system behavior |
| 6: extensions | Channel/thread/graph-transport and ownership tests documented by subsystem | Full distributed failure, shared-resource, synchronization and extension-ABI acceptance |
| 7: GC/debugger | Root/OOM tests, fault policies and source-aware debugger API | Coverage across every new subsystem; concurrency, finalization, deadlock and intercepted-channel diagnostics |
| 8: JIT | Native Linux subset; foreign-ISA encoders and Windows/macOS protocol fixtures | Full advertised AVM1/native equivalence and actual runtime acceptance on each CPU/OS; fixtures are not foreign-platform runs |
| 9: platforms/release | Linux build/test infrastructure and target-specific adapters | Actual Windows/macOS/Android build/run/device/endurance acceptance, complete compatibility matrix and reproducible release gates |

## v0.583 verification / Проверка v0.583

Default Linux Release: 1235 passed, four explicit platform/service skips out of
1239 registered. Eight separate real-SDL frontend tests passed under X11/Xvfb,
including input/pixels, both execution modes, prepared StopSounds, a 350 ms
render-thread stall and simulated 8192-frame obtained-quantum policy. The latter
uses real SDL with wrapped geometry, not an Android device. Four new stop-binding
core tests also passed ASan/UBSan with leak detection disabled. The no-device
C89 tutorial reports `pending=0 stops=1`. SDL SDK queue geometry checks passed.

Android ARM64 v0.583: real NDK/SDL native build and test APK with signature and
16 KiB ELF/ZIP alignment checked. The user reported sporadic knocks/crackling
in v0.582 on Android. The worker/reserve change addresses an identified underrun
scenario; actual listening confirmation on that phone remains outstanding.
No Windows/macOS runtime or stage-9 completion is inferred.

## Current Linux verification / Текущая проверка Linux

v0.582: complete default Linux Release C89/C++98 build, including examples;
1230 passing tests and 4 explicit skips out of 1234 registered. Skips require
native macOS ARM64/Intel, native Windows unwind, or an external speech SSIP endpoint.
The separate lean AGG/JIT SDL2 player passes five explicit frontend checks under
X11/Xvfb, including real keyboard/mouse injection and pixel changes in both modes.
Both delivered SWFs complete 120 frames in interpreter and JIT; interactive JIT
reports native=120/interpreter=0/fallbacks=0. SDL disk output in both modes contains
PCM8/PCM16/ADPCM and expected mono/stereo tone frequencies. Dummy/disk audio checks
are not physical listening acceptance. Core default tests have desktop SDL OFF.

Windows x64: real MinGW GCC13 C89/C++98 PE32+ player build and DLL-import inspection;
no actual Windows runtime run. Android arm64-v8a/API23: NDK r27c + SDL2 2.30.0,
SDLActivity Java/Dex and test-signed APK, 16 KiB ELF/ZIP alignment and signature
verified; no actual Android device/emulator execution. SDK sensor support disabled
for the older SDL2 source's NDK27-incompatible ALooper_pollAll call. This does not
replace native platform/device/endurance gates.

Полная core-сборка Linux: 1230 успешных, 4 пропуска из 1234. Отдельный player:
5/5 X11/Xvfb проверок, реальные input/pixel изменения и interpreter/JIT обоих SWF.
PCM записан настоящим SDL disk backend и проверен по частотам в обоих режимах.
Windows/Android собраны, но на целевых ОС не запускались. Подпись/ABI/выравнивание
APK и кросс-компиляция не доказывают нативную совместимость.

## Current work and next boundary / Текущая работа и следующий рубеж

v0.582 connects the desktop host to a documented HostInput extension and root
PCM/ADPCM event queues; --mute avoids device opening. Adds reproducible lean
player profile/cross-toolchain, Android SDLActivity test menu and two standalone
SWF fixtures. [EN](PLAYER_TEST_BUILDS.md) / [RU](PLAYER_TEST_BUILDS.ru.md).
Wall-clock video and PCM FIFO clocks remain independent. Next: full Flash
Key/Mouse/button/event routing, StopSounds/AS Sound integration into this player
mixer, drift/latency/underrun policy, sprite sound/linkage and stream codecs.
Native Windows/Android/macOS device runs and complete reference acceptance remain
open. All nine stages remain open.

v0.582 даёт настоящие тестовые player binaries/APK, input snapshot и root event
звук. Полный стандартный input/audio API, A/V синхронизация и приёмка целевых
платформ ещё открыты; номер версии не означает завершение стадий.

## v0.577 desktop checkpoint / Десктопный запуск

The optional [SDL2 desktop host](DESKTOP_FRONTEND.md) adds OS launch adapters,
JIT/interpreter selection, quality, monotonic frame pacing and playlists.
Stage bounds and a render-only viewport preserve authored transforms.
Linux X11/software presentation is exercised with real windows, including
RGBA pixels, close events and FWS/CWS playback. Cairo core viewport/shape/RECT/
raster tests also pass (4/4); this build does not require SDL.
The eight documentation-viewer tests pass. Foreign-OS launch adapters are
implemented but not native-tested. All nine stages remain open.

Добавлен [десктопный host](DESKTOP_FRONTEND.ru.md). Проверены окно X11,
пиксели, закрытие, FWS/CWS, JIT/interpreter и управление темпом.
Cairo: 4/4; просмотрщик документации: 8/8. Windows/macOS требуют
нативного прогона, Android — Activity/surface frontend. Это не закрывает
полноту SWF, input, звуковой clock и платформенную приёмку стадии 9.

Historical v0.577 desktop-enabled Linux profile: 1140 passed and 3 explicitly skipped
(native macOS ARM64/Intel and Windows unwind), 1143 registered tests total.
This run disables examples and is not directly comparable to v0.576's count.
Исторический Linux-профиль v0.577 с desktop: 1140 успешных, 3 пропущенных, всего 1143;
examples выключены, поэтому число тестов отличается от предыдущего профиля.

## v0.578 incremental event mixing / Порционное смешивание событий

Independent software polyphony now combines prepared PCM/ADPCM snapshots with
per-ID stop/no-multiple, loop/envelope progress, fixed geometry and bounded voice
capacity. Wide summation clips once; reads allocate nothing and survive source
reload/GC/context/runtime destruction. See [EN](SWF_SOUND_MIXER.md) /
[RU](SWF_SOUND_MIXER.ru.md) and the annotated C89 tutorial.

Five focused mixer/tutorial tests pass in the Linux minimal-JIT ASan/UBSan build
(LeakSanitizer disabled). This includes injected OOM cleanup, all ADPCM widths,
packet/chunk boundaries, exact native file PCM and strict C89 use. Source links,
debugger tutorial contracts and the new example's line comments/catalog/anchors
are checked separately. No native device or foreign-OS acceptance is claimed.

Это развитие стадий 3/4/7, а не закрытие любой из девяти стадий. Следующие
звуковые рубежи: фактическая доставка исполняемых timeline-событий в host mixer,
разные частоты/каналы, системный device clock, streaming codec/A/V integration.
Общее число тестов не заменяет эти условия и оригинальные Flash-эталоны.

## v0.579 root frame sound delivery / Звуки фактического входа в кадр

Root DefineSound/StartSound now import and deliver ordered event ordinals through
a synchronous host callback before DoAction. Visual seek reconstruction does not
replay skipped sound frames. Loaded source bytes are checked against the imported
graph, including after reload or transport; callback pointers remain host-local.
The C89 mixer tutorial adds --timeline-demo with real frame-driven submission.
See [EN](MOVIECLIP_SOUND.md) / [RU](MOVIECLIP_SOUND.ru.md).

Следующий рубеж: sprite sound, эффективный prepared source index, resampling и
подключение к реальному device clock. Callback сам не создаёт устройство и не
синхронизирует видео/звук. Все девять стадий остаются открыты.

Final v0.579 verification: full default Linux Release build/CTest, 1216 passed
and 4 explicit skips out of 1220. Focused timeline/OOM/transport/validation/tutorial
tests: 5/5 in ASan/UBSan, LeakSanitizer disabled. No SDL window, audible device
or foreign native rerun is claimed. Source links and example contracts pass.

## v0.580 output conversion / Преобразование выхода

A separate configured creation ABI enables rational-phase linear conversion to
5512..192000 Hz mono/stereo. The legacy strict entry point and info ABI are
unchanged. Different source geometries, loop duration, buffered EOF, slot reuse,
stop/no-multiple and final wide-sum saturation are covered. Eight focused
conversion/timeline/tutorial tests pass in Linux ASan/UBSan (LeakSanitizer off).
An actual strict-C89 client compiled against the v0.579 mixer header also links
and runs against the new library. [EN](SWF_SOUND_CONVERSION.md) /
[RU](SWF_SOUND_CONVERSION.ru.md). This is software conversion without a low-pass
anti-alias filter or device/foreign-platform acceptance.

Final v0.580 verification: full default Linux Release C89/C++98 build/CTest,
1223 passed and four explicit native-macOS/native-Windows/SSIP skips (1227 total).
Eight focused tests pass in minimal-JIT ASan/UBSan with LeakSanitizer disabled.
Legacy-header ABI client, source links, guide anchors, tutorial line comments/no
assertions, catalog entries and debugger tutorial contracts pass separately.
No native audio device, SDL window or foreign-platform rerun is claimed.

## v0.584 diagnostic checkpoint

Native frontend-only changes: optional bounded SDL-input capture and Android
private read-only ZIP export. Ten frontend checks passed: nine SDL/X11/Xvfb checks plus bounded storage/error
verification. Actual accepted WAV in both interpreter/JIT preserves expected stereo
tones and matching non-clipped peaks; StopSounds markers are recorded. The storage
fixture also passed strict C89 compilation and ASan/UBSan (leaks disabled).
Windows x64 and Android ARM64 player builds passed; APK Java/Dex, signature and
16 KiB alignment checks passed. No new core semantic acceptance or actual Android
listening/provider run is claimed.
[EN](PLAYER_AUDIO_DIAGNOSTICS.md) / [RU](PLAYER_AUDIO_DIAGNOSTICS.ru.md).

## v0.585 Android device evidence

Samsung SM-S931B / Android16 user traces establish excessive AAudio queue drain,
not general Android compatibility. The manifest selects the blocking AudioTrack
backend. APK metadata/DEX checksums/signature/16 KiB alignment and identical native
payloads verified; native rebuild, runtime playback and the new driver's phone
acceptance remain pending. [Investigation](PLAYER_AUDIO_DIAGNOSTICS.md).
