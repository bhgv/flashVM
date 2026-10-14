# Условия окончательной приёмки / Final acceptance gates — v0.125

Основание: [ROADMAP.md](ROADMAP.md). Все девять стадий остаются открытыми.
Это состояние реализации и доказательств, а не оценка процента готовности.

| Стадия | Проверенная часть | Что требуется для завершения |
| --- | --- | --- |
| 1 | Counted UTF-16 values/property/member/global names, selected Object/Array/Number semantics | Full arguments/type conformance, Array size boundaries, full API inventory and historical/versioned conversion comparisons |
| 2 | Selected Function2/preloads/arguments, inheritance/super, numeric and member/variable actions | Complete action/version/scope/exception semantics with compiler-generated SWFs and a reference oracle |
| 3 | Frame actions/calls, sprites/exports, selected scene controls, target scopes/paths, indexed fields, shapes/bitmaps and clip APIs | Complete SWF display list/resources/masks/drag/load/text creation and event ordering |
| 4 | Selected raster/bitmap/filter/geometry/text operations and bundled dependencies | Complete text/font integration, filter/blend semantics, audio/video/streaming and historical comparisons |
| 5 | Selected standard classes and host input/network/URL request bridges | Complete AS1/AS2 inventory, XML/XMLSocket/LoadVars/SharedObject, loaders/timers/events/system APIs |
| 6 | Selected graph/resource/worker/channel/mutex transport, nested fault context | Distributed failures/ownership reconciliation and deadlocks, all built-in graph cases and extension-module acceptance |
| 7 | Selected GC roots/cycles/native captures/finalizers, freeze/peek/local wait cycles | All-subsystem fault/stress acceptance, full distributed debugger graphs and complete memory-tool evidence |
| 8 | Interpreter only; JIT selection rejects configuration | Actual ARM32/AArch64/x86/x64/RV32/RV64 machine code, ports and interpreter-equivalence acceptance on each target |
| 9 | Linux x86_64 AGG and Cairo Release builds/runs; portable and restricted sanitizer profiles | Real Windows/macOS/Android builds/runs, supported display adapters, source-vendored static dependency audit, reproducible release archives and stages 1–8 acceptance |

## Проверки этой среды / Evidence from this environment

| Среда | Доказательство v0.125 | Ограничение |
| --- | --- | --- |
| Linux x86_64 GCC/G++ 13.3, AGG/fonts/codecs/SQLite | 292/292 core CTest tests plus the optional runner test, strict C90/C++98 | Examples excluded; passing subset does not certify full Flash compatibility |
| Linux x86_64 Cairo Release/fonts/codecs/SQLite | 292/292 core CTest tests with assertions retained | Same compatibility gaps; no other OS runtime proof |
| Linux headless ASan/UBSan | 4/4 focused Array/AVM1 coercion checks; earlier focused evidence in feature docs | /proc restrictions require disabled stack ASan and LeakSanitizer; no full stack/leak certification |
| NO_OS_PORTS portable profile on Linux | Array/AVM1 coercion checks 4/4 | TCP excluded; this is still Linux execution |
| Windows | No runtime evidence in this session | No Windows runner/compiler/emulator available |
| macOS | No runtime evidence in this session | No macOS runner/SDK available |
| Android | No runtime evidence in this session | No Android device/NDK/emulator available |

JIT, media and unsupported presentation keys must continue to fail configuration
until real implementations exist. A platform key, source adapter, cross-compile
or successful subset suite is not a substitute for execution on that platform.

Counted constructor/stack names and parameter aliases have since been implemented
and tested. Primitive/type acceptance still requires full Array boundaries and
historical profiles. AVM1/display work supplies actual roots for external type/
operator comparisons; ten original probes now match. Complete action, resource,
object/method and version inventories remain necessary. Stages 6/7 accompany
all changes. These checkpoints do not close any stage; acceptance requires full
reference behavior and real target-platform execution beyond the passing subset.

## Русское пояснение

Сборки подтверждают выполнение перечисленных Linux-сценариев. Они не закрывают
пункты реализации, которых ещё нет, и не заменяют запуск на другой ОС. Особенно
отдельно остаются JIT всех шести ISA, мультимедиа и полный SWF/AS1/AS2 контракт.
Финальный выпуск стадии 9 допускается только после реализации и приёмки всего
роудмапа, реальных платформенных запусков и документированного воспроизводимого
архива исходников с полной историей Git. Текущий архив — контрольная точка v0.83,
а не итоговый SWF 8 release. Generated logs/evidence остаются вне Git и источников.

## v0.84 checkpoint

Counted native constructor/stack-name gates above are implemented and covered
by native_counted_construction. Linux AGG/Cairo Release now pass 247/247 each;
focused headless sanitizer checks 4/4 and portable new test 1/1. Earlier v0.83
figures remain historical evidence. Parameter aliases and remaining type/API
conformance keep stage 1 open. The active user endpoint is stage 8; stage 9
platform/release acceptance remains separate and is not being claimed complete.

## Current endpoint and evidence — v0.243 / Текущая цель и проверки

The user's active endpoint is all nine roadmap stages, including real platform
acceptance. Earlier statements restricting the endpoint to stage 8 are historical
and superseded. Stages 1–9 remain open. Shared value-based C execution services
and prepared native bodies support future generated code; no machine-code JIT
backend exists yet. Linux AGG/Cairo each pass 450 current tests; 23 unchanged
SWF<=8 reference traces match per profile. Focused heap/global ASan+UBSan evidence
and its stack/leak limits are recorded in the feature reports.

This workspace provides Linux execution. No Windows compiler/runner, macOS SDK/
runner, Android NDK/device/emulator or requested-ISA emulator was found. Actual
cross-platform and JIT execution gates therefore have no new runtime evidence.
Portable declarations or configuration checks do not close these gates.

Текущая команда пользователя охватывает все девять стадий, а не только стадию 8.
Проверенные C-службы и Linux-сборки не означают готовность JIT или финальный
выпуск. Для приёмки Windows/macOS/Android нужны реальные среды сборки и запуска,
которых в этой рабочей среде нет. Остальные пробелы реализации перечислены в
реестре совместимости и остаются открытыми.

## v0.247 — reproducible source/history packaging

[SOURCE_ARCHIVE.md](SOURCE_ARCHIVE.md) documents a reusable exporter for committed
sources and complete reachable Git history, with fixed metadata, isolated staging,
strict fsck/count/CRC checks and two byte-identical Linux exports. Runtime evidence
remains v0.246: 454 tests per AGG/Cairo profile, 23 original traces per profile,
seven focused heap/global ASan+UBSan checks. This packaging-only checkpoint does
not change runtime behavior or close any platform/JIT/compatibility stage.

Добавлен воспроизводимый экспорт истории Git; это проверенная часть выпуска,
а не завершение стадии 9. Реальные проверки остальных ОС ещё необходимы.

## v0.249 — current checkpoint

The runtime C-service/decomposition checkpoint is v0.246 with v0.248 named field
constants: 454 tests per AGG/Cairo profile, 23 unchanged reference traces per
profile, seven focused heap/global ASan+UBSan checks. v0.247/v0.249 add and verify
reproducible committed-source/reachable-history export, including SHA-256 object
format preservation. This is progress on release packaging only. Complete stages
1–8, actual JIT equivalence and non-Linux execution still gate final stage 9.

## v0.254 — current execution and reference work

Scoped/qualified Delete2, rooted script getter/setter entry, retained MovieClip
class-prototype string identity and common C primitive conversions extend the
interpreter/generated-body interface. The expanded original SWF<=8 corpus is 25
cases, including the correctly hosted three-frame delete probe. Feature reports
record exact build/memory evidence and remaining display-path/type/GC gaps.
All stages 1–9 remain open. Reproducible source/history packaging does not replace
full JIT implementations/equivalence or real Windows/macOS/Android execution.
