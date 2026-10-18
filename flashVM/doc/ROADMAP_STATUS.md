# FlashVM stage status / Состояние стадий — v0.577

This is a bounded review of existing implementation reports and the current
desktop-host work. It does not replace a complete per-API/SWF conformance matrix
or certify unexecuted platforms. All nine stages remain open. Existing test
counts are historical unless explicitly rerun for this revision.

Это сводка существующих отчётов и текущего десктопного host, а не
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

## Current Linux verification / Текущая проверка Linux

v0.576: complete current-profile C89/C++98 build; 1204 passing tests and four
explicit skips out of 1208 registered. Skips: native macOS ARM64/Intel and Windows
unwind acceptance, plus the conditional speech SSIP endpoint. The new MP3 prefix,
encoded snapshot, allocation-failure and real tutorial tests pass. No new native
foreign-OS or sanitizer run is claimed.

Полная текущая Linux-сборка: 1204 успешных теста, 4 явных пропуска из 1208.
Пропуски не засчитываются в нативную приёмку других ОС или speech-сервиса.

## Current work and next boundary / Текущая работа и следующий рубеж

v0.576 develops stages 3/4/7: retain encoded stream blocks independently of the
VM and inspect the SWF MP3 count/seek prefix safely. It enables external decoder
integration but supplies no compressed decoder, seek scheduler or audio clock.
Next streaming boundary: stateful codec decoding with authentic input/oracles,
then sample scheduling against MovieClip frame progression. Do not confuse copied
wire SampleCount with a verified decoded frame count.

v0.576 развивает стадии 3/4/7: независимое хранение сжатых блоков и безопасный
разбор MP3-префикса. Следующий рубеж — настоящий stateful decoder с реальными
входами/эталонами, затем планирование samples по timeline MovieClip. Это не
закрывает остальные стадии и не заменяет нативные проверки других ОС.

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

Current desktop-enabled Linux profile: 1140 passed and 3 explicitly skipped
(native macOS ARM64/Intel and Windows unwind), 1143 registered tests total.
This run disables examples and is not directly comparable to v0.576's count.
Текущий Linux-профиль с desktop: 1140 успешных, 3 пропущенных, всего 1143;
examples выключены, поэтому число тестов отличается от предыдущего профиля.
