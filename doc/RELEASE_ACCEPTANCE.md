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

## v0.277 — value-based SetVariable names

Both Linux profiles pass 529 tests, seven affected memory checks pass, and all
26 original SWFs match per profile. The shared C handler owns conversion and
roots; the interpreter only adapts operands. Machine-code JIT and actual
Windows/macOS/Android runs remain absent. Все стадии 1–9 остаются открытыми.

## v0.278 — qualified declaration read semantics

Both Linux profiles pass 532 tests, six focused memory checks pass and 26 original
SWFs match per profile. Qualified undefined reads now perform assignment while
non-undefined reads preserve the property. Machine-code JIT and real runs on
Windows/macOS/Android remain absent. Все стадии 1–9 остаются открытыми.

## v0.279 — shared primitive boxing and member setters

Both Linux profiles pass 536 tests; six focused memory checks and the final
constructor-order callback check pass. All 26 original SWFs match per profile.
Temporary member wrappers dispatch script setters and retain inputs through GC.
Machine-code JIT and Windows/macOS/Android runs remain absent. Все стадии открыты.

## v0.280 — shared With boxing

Both Linux profiles pass 539 tests, five focused memory checks pass and all
26 original SWFs match per profile. With uses value-based construction and
retains its parent through GC. Machine-code JIT and real Windows/macOS/Android
verification remain absent. Все стадии 1–9 остаются открытыми.

## v0.281 — named method calls and receiver boxing

Both Linux profiles pass 543 tests, eight focused memory checks pass and all
26 original SWFs match per profile. Named calls retain the selected prototype
and getter result and do not consult stale script caches in loaded execution.
Machine-code JIT and actual Windows/macOS/Android runs remain absent.
Все стадии 1–9 остаются открытыми.

## v0.282 — value-based CastOp

Both Linux profiles pass 546 tests, four focused memory checks pass and all
26 original SWFs match per profile. Primitive casts retain constructor side
effects without replacing the tested original value with a wrapper. Real JIT
and Windows/macOS/Android validation remain absent. Все стадии 1–9 открыты.

## v0.283 — value-based InstanceOf

Both Linux profiles pass 550 tests, eight focused memory checks pass, and all
26 original SWFs match per profile. InstanceOf and object CastOp share rooted
coercion and own prototype data semantics. Interface metadata snapshots and
other compatibility gates remain open. JIT and real Windows/macOS/Android runs
remain absent. Все стадии 1–9 остаются открытыми.

## v0.284 — frozen interface graphs

Both final Linux profiles pass 555 tests, nine focused memory checks pass and all
26 original SWFs match per profile. Snapshot edges and installed empty lists
survive graph validation, GC, memory/file channels and localhost broker transfer.
New snapshot graphs require a schema-9-capable reader; ordinary graphs remain
schema 8. Real JIT and Windows/macOS/Android runs remain absent.
Все стадии 1–9 остаются открытыми.

## v0.285 — ordered Extends service

Both Linux profiles pass all 559 tests; AGG required executable-permission repair
and reruns for launch failures, with original and retry logs retained separately.
Four focused heap/global ASan+UBSan checks pass (no stack/leak coverage), and all
26 original SWFs match per profile. Loaded Extends now shares a value-based C ABI
with ordered boxing and rooted getter/setter dispatch. Actual machine-code JIT
and Windows/macOS/Android runs remain absent. Все стадии 1–9 остаются открытыми.

## v0.286 — documented operand and literal constants

Both Linux profiles pass all 559 tests after restoring executable permissions
for six AGG and two Cairo launch failures. Original and retry logs remain separate.
Six focused heap/global ASan+UBSan checks pass, without stack/leak coverage;
all 26 original SWFs still match per profile. This constants-only checkpoint
adds no compatibility or platform claim. Все стадии 1–9 остаются открытыми.

## v0.287 — shared numeric stepping

Both final Linux profiles pass all 562 tests; Cairo required permission repair
and reruns for two launch failures. Three focused heap/global ASan+UBSan checks
pass, without stack/leak coverage, and all 26 original SWFs match per profile.
Increment/Decrement use the shared C89 service with rooted numeric coercion and
aliased-output failure atomicity. Actual machine-code JIT and real runs on
Windows/macOS/Android remain absent. Все стадии 1–9 остаются открытыми.

## v0.288 — named coercion syntax and whitespace

Both Linux profiles pass all 562 tests. AGG required reruns of 16 launch failures;
original and retry logs are retained separately. Five focused heap/global
ASan+UBSan checks pass (no stack/leak coverage), and all 26 original SWFs match
per profile. Named numeric syntax and whitespace constants preserve the existing
accepted set. Real JIT and Windows/macOS/Android verification remain absent.
Все стадии 1–9 остаются открытыми.

## v0.289 — value-based property enumeration

Both Linux profiles pass all 565 tests after executable-launch repairs and
reruns; original and retry logs remain separate. Four focused heap/global
ASan+UBSan checks pass, without stack/leak coverage, and all 26 original SWFs
match per profile. Loaded primitives/empty stacks now enumerate without errors,
using undefined sentinels; name coercion, GC/exceptions, counted keys, allocation
failures and callback-free snapshot construction are covered. Native key order
is unchanged and historical ordering remains open. Real machine-code JIT and
Windows/macOS/Android runs remain absent. Все стадии 1–9 остаются открытыми.

## v0.290 — bounded RandomNumber execution

Both Linux profiles pass all 568 tests; AGG required executable-launch repairs
and reruns for two failures. Three focused heap/global ASan+UBSan checks pass,
without stack/leak coverage, and all 26 original SWFs match per profile. The new
Action/C service covers saturating bounds, rooted coercion, exception/allocation
atomicity and shared Math state including graph transport. Existing Math.random
sequences are preserved; historical player/locale RNG equivalence remains open.
Machine-code JIT and real Windows/macOS/Android runs remain absent.
Все стадии 1–9 остаются открытыми.

## v0.291 — elapsed monotonic GetTime

Both final Linux profiles pass all 571 tests without launch failures in those
final runs. Three focused heap/global ASan+UBSan checks pass (no stack/leak
coverage), and all 26 original SWFs match per profile. Deterministic clock
fixtures cover SWF4–8 Action dispatch, context origins, load retention, unsigned
wrap, clock rollback, failure/recovery and allocation-free reads. A separate
fixture runs the actual Linux clock adapter. Windows/macOS/Android adapter
sources are unverified on target; real JIT and all nine acceptance stages remain
open. Все стадии 1–9 остаются открытыми.

## v0.292 — TargetPath and dispatch catalog

Both Linux profiles pass all 574 tests after executable permission repairs and
reruns for seven AGG and one Cairo launch failures; original and retry logs are
retained separately. Four focused heap/global ASan+UBSan checks pass, without
stack/leak coverage, and all 26 original SWFs match per profile. TargetPath has
C89/Action, rooted boxing, GC/exception, counted-name and allocation coverage.
The new source audit finds 92 dispatch references in a pinned 100-action catalog;
its require-all gate correctly fails for eight missing actions. Source presence
is not behavioral certification. Real JIT and actual Windows/macOS/Android runs
remain absent. Все стадии 1–9 остаются открытыми.

## v0.293 — value-based frame availability

Both Linux profiles pass all 578 tests; AGG required executable permission
repairs and reruns for 14 launch failures. Original and retry logs remain separate.
Four focused heap/global ASan+UBSan checks pass (no stack/leak coverage), and all
26 original SWFs match per profile. Both wait Actions cover strict value parsing,
versioned coercion, alias/allocation errors, callback scope/GC, whole-record skips,
nested bodies and terminal overshoot. The pinned catalog has 94 present source
references and six absent entries; current timelines are fully loaded and partial
streaming remains absent. Real JIT and actual Windows/macOS/Android runs remain
absent. All nine acceptance stages remain open. По новой команде пользователя
архивы исходников/локального Git больше не создаются; выдача идёт через зеркало.

## v0.294 — atomic native MovieClip detachment

Both Linux profiles pass all 580 tests without launch failures in these full
runs. Three focused heap/global ASan+UBSan checks pass (no stack/leak coverage),
and all 26 original SWFs match per profile. Prepared ownership-map publication
preserves graph state on allocation failure at positive and negative depths;
C89 host/SWF4–8 fixtures cover counted names, aliases, repeated removal and missing
names. Stale slot identity protection is exercised separately. RemoveSprite,
full rename/unload/removed-target semantics, real JIT and target-platform runs
remain open. Все стадии 1–9 остаются открытыми; зеркало — текущий способ выдачи.

### v0.295 — counted clip replacement / Полные имена при замене клипа

Depth replacement and scene rename preserve embedded-NUL binding lengths and
unrelated prefix aliases; replacement tolerates a missing _name. Covered by
movieclip_remove_names and movieclip_remove_allocation, including host/SWF4–8
replacement and direct scene rename. See MOVIECLIP_REMOVAL_ATOMICITY.md.
Поиск связи при замене и переименовании больше не усекает имя по NUL.
Full rename semantics, RemoveSprite and all roadmap stages remain open.

### v0.297 — explicit forced clip detachment / Явное отсоединение клипа

The C89 fvm_exec_detach_clip service exposes native forced scene removal without
bytecode or script method lookup. Host/SWF4–8 and positive/negative-depth fixtures
cover override bypass, invalid objects and repeated calls; allocation injection
checks link/root preservation through this public entry. See CLIP_DETACH_SERVICE.md.
Принудительное отсоединение доступно из C; Action RemoveSprite, исторические
ограничения глубин, события и восстановление цели остаются открытыми.

### v0.298 — shared removal implementation / Декомпозиция отсоединения

The native callback and C service now share the single transaction in
movieclip_remove.cpp, using common raw-state/depth helpers in movieclip_storage.hpp.
No graph/callback identifiers, depth policies or Action coverage change.
Транзакция вынесена из основного MovieClip-файла; проверки состояния и глубин общие.
See CLIP_DETACH_SERVICE.md for scope and validation limits.

### v0.299 — named execution roots / Имена корней общих обработчиков

Nine remaining handlers use documented role/count macros; recursive equality has
its own root layout and named recursion bound. Call-frame's unused third root is
removed. Callback arities remain separate from GC sizes; C ABI and action coverage
are unchanged. See EXECUTION_ROOT_LAYOUTS.md for roles and validation boundaries.
Оставшиеся числовые индексы корней заменены смысловыми именами; все стадии открыты.

### v0.300 — missing unary AVM1 operands / Унарные операции на пустом стеке

Audited loaded unary operations now convert undefined instead of failing before
the shared C service. Raw-byte fixtures cover SWF4–8 and strict host execution;
allocation injection verifies root/execution restoration. See AVM1_UNARY_MISSING.md.
Загруженный SWF получает исторические преобразования undefined; другие случаи
недостатка операндов, полная семантика AVM1 и все стадии остаются открытыми.

### v0.301 — missing binary operands / Недостающие бинарные значения

Twenty-four audited loaded binary actions now supply undefined below existing
top operands, preserving right/left position and shared C coercion semantics.
Raw-byte fixtures cover zero/one values, SWF4–8, strict host mode, collecting
valueOf and allocation failures. See AVM1_BINARY_MISSING.md. Other stack policies
and all roadmap stages remain open. Порядок аргументов проверен отдельно.

### v0.302 — missing stack/control operands / Стек и управление без значения

Loaded Pop/Duplicate/Swap/Return/Throw/If/Trace now follow audited undefined
missing-pop behavior. Raw bytes cover host strictness and SWF4–8, pending thrown
undefined, function return, trace text and malformed If records.
See AVM1_CONTROL_MISSING.md. Остальные политики стека и все стадии открыты.

### v0.303 — missing member/string operands / Члены и строки без значений

Loaded StringExtract/MBStringExtract, GetMember/SetMember and Delete now prepare
undefined below existing operands. Fixtures cover all missing counts, host/SWF4–8,
collecting name coercion after dropping globals, and three-operand allocation
failures. See AVM1_MEMBER_STRING_MISSING.md. Full semantics and all stages stay open.
