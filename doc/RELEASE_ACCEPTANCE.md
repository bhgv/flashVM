# Условия окончательной приёмки / Final acceptance gates — v0.117

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

| Среда | Доказательство v0.117 | Ограничение |
| --- | --- | --- |
| Linux x86_64 GCC/G++ 13.3, AGG/fonts/codecs/SQLite | 283/283 core CTest tests plus the optional runner test, strict C90/C++98 | Examples excluded; passing subset does not certify full Flash compatibility |
| Linux x86_64 Cairo Release/fonts/codecs/SQLite | 283/283 core CTest tests with assertions retained | Same compatibility gaps; no other OS runtime proof |
| Linux headless ASan/UBSan | 4/4 focused Array/slice checks; earlier focused evidence in feature docs | /proc restrictions require disabled stack ASan and LeakSanitizer; no full stack/leak certification |
| NO_OS_PORTS portable profile on Linux | Array/slice checks 4/4 | TCP excluded; this is still Linux execution |
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
