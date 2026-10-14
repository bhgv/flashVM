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

## v0.255 — explicit-base display-path service

The common deletion interface now reuses MovieClip path resolution for slash
prefixes. This extends the C/generated-body foundation; it does not implement JIT
code generation, full interpreter scope retries or non-Linux release acceptance.
Все девять стадий остаются открытыми; проверка путей не закрывает эти требования.

## v0.256 — scoped display-path lookup

Delete2 now calls a shared scoped-target resolver for slash prefixes. Complete
path retries, explicit display-root/this handling, collector roots and allocation
failure behavior are covered by focused fixtures. Full variable-path grammar,
JIT backends/equivalence and real non-Linux execution still gate acceptance.
Все стадии 1–9 остаются открытыми; число проверок не заменяет итоговую приёмку.

## v0.257 — common Delete2 reference service

Delete2 coercion, qualified reference parsing and scoped final-member presence
now live in C89 services usable from native bodies. This corrects overly broad
v0.256 parsing while reducing interpreter dispatch. GC/exception/allocation
fixtures accompany the change; all roadmap stages and final release gates remain
open. Обновление общего C-слоя не заменяет JIT и реальные проверки остальных ОС.

## v0.258 — qualified variable reads

Counted GetVariable and Delete2 owner lookup now share the versioned C path phase.
Fallback/undefined-presence and rooted callback behavior have focused fixtures;
this does not close general assignment/call semantics, JIT or platform gates.
Все стадии роудмапа остаются открытыми; итоговая приёмка не подменяется фикстурами.

## v0.259 — shared variable assignment

SetVariable now uses explicit C inputs and getter-free binding presence, with
qualified scoped writes and callback/allocator coverage. This extends the common
execution layer while leaving historical this/case/removed-target behavior, full
JIT equivalence and non-Linux runtime gates open. Все стадии ещё открыты.

## v0.260 — plain lookup and With receiver retention

Plain variable reads and callable receiver selection now use explicit C inputs.
A getter's scope mutation/GC no longer loses its original With classification.
Focused C/interpreter fixtures accompany the decomposition. Complete scope/case/
this equivalence, actual JIT backends and non-Linux execution still gate release.
Все стадии 1–9 открыты; общий C-слой не означает завершения JIT или выпуска.

## v0.261 — loaded catch assignments and original register probe

Loaded named catch uses normal scoped writes and retains its assigned value;
host temporary bindings remain compatible. Setter GC/error and scope restoration
fixtures accompany it. The unchanged external SWF<=8 corpus expands to 26 cases.
Full catch/this/encoding and all JIT/platform/release acceptance gates remain open.
Все стадии 1–9 остаются открытыми; новые проверки не означают завершения выпуска.

## v0.262 — common local declaration service

Declarations and named function publication share a C89 managed-value interface,
including With own-slot selection and rooted callback/error/allocator behavior.
Full scope/prototype/call equivalence, all JIT backends and non-Linux execution
remain release gates. Все стадии 1–9 остаются открытыми.

## v0.263 — shared scoped function calls

Named calls now retain qualified receivers and defining prototypes, and loaded
calls avoid stale script-registry fallback. C89, interpreter, GC, exception and
allocation-failure fixtures accompany the service. Full call/scope equivalence,
six machine-code backends and non-Linux execution remain release gates.
Все стадии 1–9 открыты; перенос обработчика не означает завершения JIT.

## v0.264 — shared named construction and fresh-instance lifetime

Loaded construction now uses literal scope resolution through the common C89
service. A prototype-getter GC lifetime defect has a reproducing memory check and
an allocation-free temporary root. Semantic root/count/depth constants are named.
Complete construction/boxing semantics, six machine-code backends and all target
platform executions remain release gates. Все стадии 1–9 остаются открытыми.

## v0.265 — value-based string/character operations

Three string/character operations now have separate implementation files and C89
interfaces usable without interpreter stacks or instruction decoding. Root,
operand and Unicode constants are documented macros. Legacy byte encodings,
full standard-object/AVM1 semantics, machine-code JIT and all target-platform
runs remain release gates. Все стадии 1–9 остаются открытыми.

## v0.266 — shared string-codec constants

Internal string representation literals have named documented macros without
changing the runtime or transport format. This readability change does not close
compatibility, JIT or target-platform acceptance gates. Все стадии 1–9 открыты.

## v0.267 — rooted binary string services

StringAdd and string comparison actions use bytecode-independent C89 services.
Input graphs and converted text remain retained across the other callback's GC;
concatenation preserves exact bytes and avoids an extra coercion-depth gate.
Complete compatibility, six machine-code backends and target-platform runs still
require acceptance evidence. Все стадии 1–9 остаются открытыми.

## v0.268 — GetVariable name coercion and SWF4 MovieClip filtering

The complete action uses an explicit C89 environment/value interface, with rooted
coercion and lookup. Generated fixtures distinguish raw reads from action result
filtering. Other display objects, legacy Enumerate/encoding, all JIT backends and
all requested platform runs remain release gates. Все стадии 1–9 открыты.

## v0.269 — plain global namespace lookup

Loaded `_global` fallback now follows ordinary binding resolution and starts in
SWF6, with versioned case spelling. Host behavior remains separate. Qualified
aliases, full system/display properties, JIT equivalence and target-platform
execution still need acceptance evidence. Все стадии 1–9 остаются открытыми.

## v0.270 — MovieClip namespace scope precedence

Target and With resolution account for concrete MovieClip namespace properties,
sharing version/case helpers and documented keyword constants. Direct member and
qualified namespace operations remain follow-up work, alongside full standard
objects, JIT and platform acceptance. Все стадии 1–9 остаются открытыми.

## v0.271 — namespace member/path consistency

Member, qualified path and Delete2 services share concrete MovieClip namespace
recognition while preserving has-own distinction and host aliases. Other virtual
display fields, full path/property semantics, machine-code JIT and requested
platform executions remain release gates. Все стадии 1–9 остаются открытыми.

## v0.272 — display path bindings and SWF4 variable names

Loaded root/parent references share explicit value lookup, preserving own fields
and private preload provenance. SWF4 timeline this remains an ordinary binding;
host compatibility descriptors remain separate. Full display/system fields,
modern this cells, JIT and target-platform runs remain release gates.
Все стадии 1–9 остаются открытыми.

## v0.273 — documented MovieClip constants

Display indices, root slots, resource bounds and serialized callback IDs have
semantic header names. Their values/wire format remain unchanged. Existing build,
transport and allocator checks accompany the refactor; all roadmap gates remain
open. Все стадии 1–9 остаются открытыми.

## v0.274 — virtual path scope operations

Both Linux AGG and Cairo profiles pass all 526 tests after restoring executable
permissions for 6 and 5 binaries respectively; initial failures were BAD_COMMAND.
Nine affected heap/global ASan+UBSan checks pass; stack/leak coverage is excluded.
The 26 unchanged original SWFs match in both profiles. Virtual paths now select
With assignment owners and block outer deletion; initialized declarations remain
own-only. Реальные JIT и Windows/macOS/Android проверки отсутствуют; все стадии
1–9 остаются открытыми.

## v0.275 — shared UTF-16 representation constants

Canonical property names and string ordering share documented bounds and masks.
Both Linux profiles pass 526 tests; four targeted memory checks and 26 original
SWFs per profile pass. Stack/leak checks, JIT and other target platforms remain
open. Все стадии 1–9 остаются открытыми.

## v0.276 — documented builtin String constants

Both Linux profiles pass 526 tests; six affected memory checks and 26 unchanged
original SWFs in each profile pass. Callback transport IDs and existing limits
are preserved. JIT machine-code generation and real Windows/macOS/Android runs
remain absent. Все стадии 1–9 остаются открытыми.
