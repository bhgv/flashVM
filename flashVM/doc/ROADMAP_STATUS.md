# FlashVM stage status / Состояние стадий — v0.593

This is a bounded review of existing implementation reports and the current
keyboard polling/listener/Selection work. It does not replace a complete per-API/SWF conformance matrix
or certify unexecuted platforms. All nine stages remain open. Existing test
counts are historical unless explicitly rerun for this revision.

Это сводка существующих отчётов и текущего клавиатурного ввода, а не
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

## v0.586 validation

Second real Android user archive: fixed AAudio overconsumption and normal source
tones, but incomplete 15-second PCM delivery. Native SDL bridge now exposes the
negotiated quantum. Nine focused host SDK/frontend checks and direct four-run
PCM trace integration use explicit dummy audio/video; these are not X11, physical
audio or Android execution. The regression also rejects the old zero-change flag.
Fresh Android ARM64 native/Java/Dex build, same v0.585 signer and 16 KiB ELF/ZIP
checks; actual new phone playback pending. No roadmap stage closes.

## v0.587 validation

Lean AGG/JIT Linux SDL Release with examples: 1225 passing tests and four explicit
platform/service skips out of 1229 registered. Generated executable permissions
were restored and the 44 initially unstarted cases all passed on rerun; no test
assertion failures remained. New direct/converted release, accepted-ring/retry
and allocation regressions also pass ASan/UBSan (leaks disabled). The allocation
fixture covers ordinary and nothrow allocation consistently. Strict C89 output
checks exercise both new APIs.

Real SDL dummy audio/video integration runs 01/03 in both modes, validates tone
amplitudes and absence of a large tone-to-silence edge in 01, and checks real
StopSounds control counts. This is not X11, hardware listening or an Android run.
Fresh NDK r27c ARM64/API23 native/SDL and JDK17/SDK35 Java/Dex APK build: signature
and 16 KiB alignment checked, persisted v0.585 signer reused. Natural source
endings and SWF files remain unchanged. Hardware release-boundary races and
physical click acceptance remain open. [Contracts](PLAYER_AUDIO_RELEASE.md).

## v0.588 phone confirmation / Подтверждение на телефоне

The user reports all three sound tests are now audibly correct on Samsung
SM-S931B/Android16 in interpreter/JIT. The latest six-run archive contains the
corrected ADPCM waveform: maximum adjacent output sample step in each 880 Hz
burst is 688 rather than the previous 1902. No software-starvation markers,
truncation or dropped diagnostic events appear. This is bounded acceptance of
these test movies, not general codec/device/endurance certification.
The archive's device.txt still says 0.587 because the Android menu/export label
was hard-coded; v0.589 reads the packaged version dynamically instead.

Пользователь подтвердил отсутствие щелчков в трёх звуковых тестах обоих режимов.
Это приёмка конкретных SWF на конкретном телефоне; она не закрывает ни одну
стадию. Старый номер в device.txt был ошибкой подписи отчёта, не основанием
выдавать его за повторный прогон старого звука.

## v0.589 keyboard polling / Опрос клавиатуры

Native per-context snapshot ABI, explicit transactional Key polling installation,
SDL virtual-key/English-ASCII translation and focus/background reset. Test 02 now
calls Key.isDown for arrows/Space and Key.getCode for its indicator. The C89
[example](../examples/embedding/keyboard_polling/README.md) demonstrates simultaneous
keys and reset. [EN contracts](KEYBOARD.md) / [RU](KEYBOARD.ru.md).
Listeners, clip/button key events, security origins, IME/text composition and
full reference/platform keyboard acceptance remain open. All nine stages remain
open; this advances input support in stages 3/5 and accompanies GC/JIT/platform
verification rather than claiming completion of the plan.

### v0.589 verification / Проверка

Lean AGG/JIT Linux SDL Release with examples: 1235 registered cases, 1231 passed
and four explicit native-macOS/native-Windows/SSIP skips after restoring missing
execute permissions and rerunning four unstarted cases. No assertion failures.
Final affected keyboard/API/C89 tutorial/SDL translation tests: 5/5. The three
core keyboard tests pass ASan/UBSan (LeakSanitizer disabled), including allocation
failure sweeps and SWF5/6 folded-name installation conflicts. Real SDL dummy
video/audio captures pass all three movies in both modes, including the updated
02's actual Key calls and preserved PCM tone levels. This is not native X11 or
physical keyboard acceptance. Android ARM64/API23 native build passes; Java/Dex
APK packaging uses the persisted signer. Phone keyboard/foreign-OS runs remain
open; the user's audio confirmation applies to the previous corrected assets.

## v0.590 Key listeners / Слушатели клавиатуры

Implemented explicit add/remove/dispatch APIs, strong registration roots, rooted
recipient snapshots, optional inherited/getter/script/native method dispatch and
ordered SDL transition buffering. Added Key.addListener/removeListener and a
read-only `_listeners` Array snapshot. Repeated SWF 02 loops keep one observer;
short taps reach both handlers before a movie tick.
[EN contracts](KEYBOARD_LISTENERS.md) / [RU](KEYBOARD_LISTENERS.ru.md),
[C89 tutorial](../examples/embedding/keyboard_listeners/README.md).

Реализованы регистрация/удаление/рассылка, GC-корни и снимок получателей;
изменения подписок действуют на следующее событие. Ошибки настоящих обработчиков
распространяются без отката уже выполненных действий. Это продвижение ввода,
GC и JIT, а не завершение стадий. Полные clip/button key events, security/IME,
точное поведение мутаций/идентичности Array оригинального Flash и аппаратная
приёмка платформ остаются открытыми. Все девять стадий открыты.

### v0.590 verification / Проверка

Lean AGG/JIT Linux SDL Release with examples: 1240 registered, 1236 passed and
four explicit native-macOS/native-Windows/SSIP skips after restoring executable
permissions for 28 initially unstarted cases and rerunning them successfully.
No assertion failures remained. Five core keyboard/snapshot/install/listener
regressions pass ASan/UBSan with LeakSanitizer disabled, including native C++
exception containment, actual GC during mutation and the 1024-listener bound.
The real SDL FIFO fixture also passes a separate strict-C89 ASan/UBSan build
linked to the same minimal-JIT library and actual SDL2, without opening devices.
The minimal NO_GRAPHICS CMake profile itself does not support the desktop target;
this separate fixture compilation is not a full sanitized desktop player build.

All three SDL dummy audio/video captures pass in interpreter/JIT, preserving
source tone amplitudes and corrected ADPCM boundaries. The shipped SWF 02 also
passes three loops and short taps in both modes with one observer and matching
counters. The C89 listener tutorial prints down/up and removed=1 deliveries=2.
Android ARM64/API23 native and Java/Dex APK builds pass, with the persisted test
signer, signature and 16 KiB ELF/ZIP alignment checked. No new physical keyboard,
Android playback or foreign-OS runtime acceptance is claimed. All nine stages
remain open.


## v0.591 explicit host keyboard focus / Выбор клипа хостом

Added [`fvm_input_set_keyboard_focus`](../include/flashvm/keyboard.h#L72) / [`fvm_input_get_keyboard_focus`](../include/flashvm/keyboard.h#L75) for one native
MovieClip, with a strong context root, private attachment validation and lazy
clearing when the clip or a physical ancestor is removed. Keyboard dispatch
snapshots the selected clip before Key listeners; callback selection changes
apply to the next event, including when a callback collects removed listeners.
The same clip in both channels deliberately receives two calls.
[EN contract](KEYBOARD_FOCUS.md) / [RU](KEYBOARD_FOCUS.ru.md).

Реализован явный C API выбора клипа, а не полный Selection: автоматический выбор
мышью/Tab, правила focusEnabled, Button/TextField и SWF ClipActions остаются
открытыми. Выбор хоста не сериализуется. Это продвижение стадий 3/5/7/8;
все девять стадий по-прежнему открыты.

### v0.591 verification / Проверка

Linux Release AGG/JIT/SDL with examples: 1241 registered, 1237 passed and four
explicit native-macOS/native-Windows/SSIP skips. Twenty-one initially unstarted
cached executables lost execute permissions; after full materialization and
permission restoration all 21 reruns pass. No runtime/assertion failures remain.

Four focused regressions pass ASan/UBSan with leak detection disabled: native
selection/GC/detached ancestry, listener mutation, dispatch allocation failure,
and real SWF6–8 interpreter/manual-JIT/automatic-JIT clip callbacks. The strict
C89 tutorial prints both listener events and both selected-clip events. The
Android ARM64/API23 NDK/SDL player builds and exports both new C ABI functions;
this is cross-build evidence, not device keyboard acceptance. RU/EN source links
and commented/no-assert tutorial checks pass. No new Windows/macOS runtime or
physical device acceptance is claimed.

## v0.592 Selection MovieClip subset / Скриптовый выбор клипа

Added explicit [`fvm_standard_selection_install`](../include/flashvm/keyboard.h#L83) with atomic publication and
versioned conflicts. Selection.setFocus accepts native clip objects or counted
dot/slash paths, applies focusEnabled/button-handler eligibility and revalidates
native attachment after getters. Selection.getFocus returns a counted native dot
path or null and shares the C host focus root. Trusted rooted entries permit GC
inside nested script getters without suspending enclosing ordinary native barriers.
[EN](SELECTION.md) / [RU](SELECTION.ru.md),
[C89 tutorial](../examples/embedding/selection/README.md).

Реализовано подмножество для хоста/SWF6–8; SWF1–5 не выдаются за поддержанный
текстовый Selection. SDL-плеер устанавливает namespace только для SWF6–8.
Поставляемый SWF 02 сам выбирает корень путём `_root`; отдельные root/Key-счётчики
сохраняются на трёх циклах в interpreter/JIT. Высота синего квадрата растёт при
отпусканиях выбранного корня; звуковые данные не менялись.

Открыты TextField/каретка/диапазоны, Button, Selection listeners и уведомления
onSetFocus/onKillFocus, автоматический выбор мышью/Tab, IME/рамки/security,
ClipActions, полная эталонная совместимость и платформенная приёмка. Все девять
стадий остаются открытыми.

### v0.592 focused verification / Целевые проверки

Linux Release AGG/JIT/SDL with examples: 1245 registered, 1241 passed and four
explicit native-macOS/native-Windows/SSIP skips. Nineteen cached executables lost
execute permissions; one bitmap allocation test exceeded its 30-second limit in
the parallel run. After permission restoration all 20 sequential reruns pass in
7.95 seconds total. No runtime/assertion failures remain. Six real SDL dummy
video/audio captures (three movies, interpreter/JIT) pass with peak 6400 and the
previous stereo amplitudes/ADPCM boundaries. This is not physical speaker testing.

Three new core regressions pass ASan/UBSan with leak detection disabled, including
installation/setFocus allocation failures, Unicode paths, real getter errors,
detachment during eligibility, versioned global conflicts, nested script GC and
SWF6–8 interpreter/manual/automatic-JIT execution. The actual interactive SWF
passes independent focus/listener counter checks over three loops in both modes.
The C89 tutorial has comments on every nonempty line and per-function guide/source
links; the catalog includes its grouped TOC and alphabetic glossary entry.

Android ARM64/API23 NDK/SDL player and Java/Dex APK build successfully. The APK
uses the persisted v0.585 test signer, with signature and 16 KiB native/ZIP alignment
checked; all three embedded SWFs match current source assets. The new C ABI symbol
is exported. This is build/runtime-fixture evidence, not physical keyboard or
new Android audio acceptance. No Windows/macOS device execution is claimed.

## v0.593 MovieClip focus notifications / Уведомления выбора клипа

Accepted script Selection identity changes now publish focus, deliver
onKillFocus(newClip), then onSetFocus(oldClip). Both endpoints remain rooted
through synchronous getters/callbacks, collection and recursive focus changes.
Nested changes persist; the outer endpoint snapshot still finishes delivery.
Missing/noncallable handlers are ignored without global-function fallback.
Actual handler errors propagate without rollback; C host setters remain silent.
[EN](SELECTION.md#focus-notifications) / [RU](SELECTION.ru.md#уведомления-фокуса).

Реализованы уведомления MovieClip, а не автоматический выбор касанием/Tab,
Selection listeners, TextField/Button, IME или полная эталонная совместимость.
Все девять этапов остаются открытыми. Результаты проверок приведены ниже.

### v0.593 verification / Проверка

Fresh Linux Release C89/C++98 AGG build with examples: 1257 registered,
1253 pass and four explicit skips (native macOS ARM64/Intel, Windows unwind,
external SSIP service). Thirteen cached test executables initially lacked execute
permission; after full byte materialization and permission restoration their
reruns pass. No assertion/runtime failure remains. The actual C89 tutorial prints
set/kill notifications with null opposite endpoints and clears focus successfully.

Three focused tests pass ASan/UBSan with leak detection disabled: endpoint
notification/reentry/GC, allocation-failure sweeps before and after publication,
and SWF6–8 interpreter/manual/automatic-JIT script callbacks collecting inside
notification. Source-link HTML/determinism and debugger tutorial checks pass.
This checkpoint supplies no new Android/Windows/macOS native execution or
reference-player ordering comparison; all roadmap stages remain open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_input_get_keyboard_focus` | function | [include/flashvm/keyboard.h:75](../include/flashvm/keyboard.h#L75) |
| `fvm_input_set_keyboard_focus` | function | [include/flashvm/keyboard.h:72](../include/flashvm/keyboard.h#L72) |
| `fvm_standard_selection_install` | function | [include/flashvm/keyboard.h:83](../include/flashvm/keyboard.h#L83) |
<!-- source-links:end -->
