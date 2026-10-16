# Условия окончательной приёмки / Final acceptance gates — v0.396

Основание: [ROADMAP.md](ROADMAP.md). Все девять стадий остаются открытыми.
Это состояние реализации и доказательств, а не оценка процента готовности.

v0.385: SetProperty width/height computes both intrinsic scales from
untransformed vector/descendant bounds using pinned rotation rules. Singular
ancestors, degenerate geometry, coercion GC/detachment, atomic OOM/errors and
interpreted/explicit/automatic native writes are covered. Named dimensions and
full geometry conformance remain open.
[EN](MOVIECLIP_DIMENSION_WRITES.md) / [RU](MOVIECLIP_DIMENSION_WRITES.ru.md).
Все стадии остаются открытыми.

Validation v0.385: all 807 AGG / 807 Cairo / 788 minimal Linux Release checks
have passing evidence after six ELF execution-permission repairs and targeted
reruns (one AGG, two Cairo, three minimal). Eight restricted heap/global
ASan+UBSan checks pass: four new dimension write/allocation/callback/native
cases plus previous dimension reads, AVM1 display properties, native indexed
reads and geometry allocation. Stack ASan and LeakSanitizer remain disabled.
The revised MovieClip constants header passes a strict C89 syntax probe.
No new original external SWF oracle or foreign-OS run is claimed.
All nine roadmap stages remain open.

v0.384: loaded Number.toString nondecimal values follow pinned Int32 truncation
and MIN-sentinel byte formatting without signed overflow. All 34 bases, fractions,
range/NaN/Infinity inputs, captured radix coercion, OOM/GC/transfer and native/
interpreted results are covered. Decimal and unloaded host formats remain.
Historical player-specific behavior is not certified.
[EN](NUMBER_LOADED_RADIX.md) / [RU](NUMBER_LOADED_RADIX.ru.md).
Все стадии остаются открытыми.

Validation v0.384: all 803 AGG / 803 Cairo / 784 minimal Linux Release checks
have passing evidence after nine ELF execution-permission repairs and targeted
reruns (four AGG, four Cairo, one minimal). Eight restricted heap/global
ASan+UBSan checks pass: four new radix/callback/allocation/native cases plus
legacy radix/conversion, C89 standard Number and native numeric regressions.
Stack ASan and LeakSanitizer remain disabled. The new scoped radix constants
header passes a strict C89 syntax probe. No original external SWF oracle or
foreign-OS acceptance was added. All nine roadmap stages remain open.

v0.383: SWF7–8 interface snapshot capture/inspection shares native AVM1 work;
budget/OOM failure preserves edges and first-installation state. Validated
loaded zero-count/primitive/SWF4–6 noops avoid direct-service root allocation.
The full 65536-reference bound, GC, saturation, allocation sweeps and interpreted/
explicit/automatic native failure/retry are covered. Host/count policies remain.
[EN](IMPLEMENTS_WORK.md) / [RU](IMPLEMENTS_WORK.ru.md).
Все стадии остаются открытыми.

Validation v0.383: all 799 AGG / 799 Cairo / 780 minimal Linux Release checks
have passing evidence after five ELF execution-permission repairs and targeted
reruns (two AGG, three Cairo; minimal passed initially). Eight restricted heap/
global ASan+UBSan checks pass: three new declaration/work cases, existing
snapshot allocation/transport, membership data/large-graph and collecting
membership-callback regressions. Stack ASan and LeakSanitizer remain disabled.
No new original external SWF oracle or foreign-OS run is claimed.
All nine roadmap stages remain open.

v0.382: loaded String.split removes its 65536-field runtime rejection;
loaded split/indexOf/lastIndexOf share nested execution work for scan positions,
comparisons and split UTF-16 copies. Large results, failed long prefixes,
finite/unlimited/saturated/nested budgets, OOM/GC/retry and interpreter/native
calls are covered. Host bounds and existing SWF delimiter/limit rules remain.
[EN](STRING_LOADED_WORK.md) / [RU](STRING_LOADED_WORK.ru.md).
Все стадии остаются открытыми.

Validation v0.382: all 796 AGG / 796 Cairo / 777 minimal Linux Release checks
have passing evidence after three ELF execution-permission repairs and targeted
reruns (two AGG, one minimal; Cairo passed initially). Eight restricted heap/
global ASan+UBSan checks pass: four new work/allocation/callback/native cases
plus legacy split/search/receiver and character-allocation checks. Stack ASan
and LeakSanitizer remain disabled. The revised String constants header passes
a strict C89 syntax probe. No new original external SWF oracle or foreign-OS
run is claimed. All nine roadmap stages remain open.

v0.381: loaded InstanceOf replaces the 4096-node rejection/linear visited
scan with an identity hash set and shared native execution accounting. Cyclic
8192-node true/false graphs, nested/unlimited/saturated budgets, OOM/GC/retry
and large interpreter/explicit/automatic native membership are covered. Array
retains its accounting through the shared scope alias. Ordinary prototype bounds
and host policy remain. [EN](INSTANCE_OF_GRAPH_WORK.md) /
[RU](INSTANCE_OF_GRAPH_WORK.ru.md). Все стадии остаются открытыми.

Validation v0.381: all 792 AGG / 792 Cairo / 773 minimal Linux Release checks
have passing evidence; AGG/Cairo passed initially, minimal required one ELF
execution-permission repair and targeted rerun. Eight restricted heap/global
ASan+UBSan checks pass: three new graph/native checks, existing membership
callbacks/allocation/data and large-sort/negative-end Array regressions.
Stack ASan and LeakSanitizer remain disabled. The new native polling and revised
membership constants headers pass a strict C89 syntax probe. No new original
external SWF oracle or foreign-OS acceptance is claimed. All nine stages remain open.

v0.380: SWF4–6 registerClass linkage names use shared Flash unit folding for
registration, C lookup/removal and native attach/duplicate/fallback selection.
Case replacement publishes constructor and new spelling atomically; GC/transfer,
OOM/retry and interpreted/explicit/automatic CallMethod are covered. Separate
global environments and exact mixed-movie provenance remain open.
[EN](OBJECT_REGISTER_CLASS_CASE.md) / [RU](OBJECT_REGISTER_CLASS_CASE.ru.md).
Все стадии остаются открытыми.

Validation v0.380: all 789 AGG / 789 Cairo / 770 minimal Linux Release checks
have passing evidence after three ELF execution-permission repairs and targeted
reruns (one AGG, two Cairo; minimal passed initially). Eight restricted heap/
global ASan+UBSan checks pass: four new registry/scene/JIT cases plus existing
C89 registry, name/transfer, sprite resources and duplicate-version checks.
Stack ASan and LeakSanitizer remain disabled. No new original external SWF
oracle or foreign-OS run is claimed. All nine roadmap stages remain open.

v0.379: indexed MovieClip width/height reads use parent-space vector bounds,
including descendant transforms and singular ancestors. Traversal snapshots and
native barriers protect reentrant removal; coercion GC, errors/OOM, aliasing and
interpreter/explicit/automatic JIT are covered. Writes and full resource/stroke/
named dimension semantics remain open. [EN](MOVIECLIP_INDEXED_SIZE.md) /
[RU](MOVIECLIP_INDEXED_SIZE.ru.md). Все стадии остаются открытыми.

Validation v0.379: all 785 AGG / 785 Cairo / 766 minimal Linux Release checks
have passing evidence after nine executable-permission repairs on eight ELF
files and targeted reruns (four repair operations AGG, three Cairo, two minimal).
Eight restricted heap/global ASan+UBSan checks pass: the four new dimension
checks plus standard MovieClip, AVM1/JIT display properties and geometry
allocation. Stack ASan and LeakSanitizer remain disabled. No new external SWF
oracle or foreign-OS runtime acceptance is claimed. All nine stages remain open.

v0.378 replaces the SWF4–6 display-path non-ASCII rejection with shared pinned
Flash UTF-16-unit name folding. Children and fallback properties/accessors retain
counted NUL/surrogates and actual spelling; SWF7–8/host stay sensitive. Keyword
folding reuses the same lookup. Real collecting own/inherited getters retain a
temporary intermediate receiver through success, exceptions and errors. OOM/retry,
allocation-free comparison and interpreted/explicit/automatic native target
selection are covered, including a genuine child/property collision.
Validation v0.378: all 781 AGG / 781 Cairo / 762 minimal Linux Release checks
have passing evidence after six ELF execution-permission repairs and failed-test
reruns (two per profile). Seven restricted heap/global ASan+UBSan checks pass:
four new Unicode path/target checks plus existing resolver, JIT target and point
case checks. Stack ASan and LeakSanitizer remain disabled. Both revised scoped
constant headers pass strict C89 syntax checking. No new external SWF oracle
or foreign-OS runtime acceptance is claimed.
MOVIECLIP_UNICODE_PATHS.md / .ru.md document duplicate-name, encoding, removed
reference and other remaining bounds. All nine roadmap stages remain open.

v0.377 removes the loaded positive-length 65,536 rejection from native/borrowed
sort/sortOn. Input capture now links roots before incremental charged appends;
descending reversal and rooted returned-index output also charge work. The
historical comparison order remains, including quadratic worst cases. Large
permutation/index sorting, budget failure before full capture, native negative
storage rejection, host limits and allocation/GC/retry are covered.
Validation v0.377: all 777 AGG / 777 Cairo / 758 minimal Linux Release checks
have passing evidence after five ELF permission repairs (2/2/1) and six failed-test
reruns, including the dependent timezone profile runner. Six restricted heap/
global ASan+UBSan checks pass: both new large-sort checks, legacy allocation,
comparator, snapshot and no-op checks. Stack ASan and LeakSanitizer remain
disabled. The revised scoped constant header passes strict C89 syntax checking.
Original external SWF traces were not rerun; no new external oracle pass is claimed.
ARRAY_LOADED_LARGE_SORT.md / .ru.md describe increased work costs and remaining
bounds. No roadmap stage or foreign platform acceptance is closed.

v0.376 removes the arbitrary 65,536 source-length rejection from loaded generic
shift/unshift/reverse/splice. Existing charged loops retain shared instruction
budgets and stop polling; native/borrowed length, raw data and flags are unchanged.
Successful 65,537-slot native/borrowed mutations, inherited accessor bypass,
SWF4–8 budget failures and allocation/GC/retry are covered. Host mutation and
generic sorting bounds remain separate named constants.
Validation v0.376: all 775 AGG / 775 Cairo / 756 minimal Linux Release checks
have passing evidence after nine ELF permission repairs and failed-test reruns
(4 AGG, 2 Cairo, 3 minimal). Five restricted heap/global ASan+UBSan checks pass,
including both new large-mutation checks, legacy unshift/splice flags and negative
end operations. Stack ASan and LeakSanitizer remain disabled. The new scoped
constant header also passes a strict C89 syntax probe.
See ARRAY_LOADED_LARGE_MUTATION.md / .ru.md. Full Array semantics, real foreign
platform execution and all nine roadmap stages remain open.

v0.375 preserves negative signed lengths through loaded slice bound conversion,
copying raw own negative keys into rooted dense native results. Both native and
borrowed sources, holes, accessor data, length-changing coercion, work limits,
allocation failure/GC/retry and source retention are covered. Signed own-key
reading is shared with negative pop without changing its deletion rules.
Validation v0.375: 773 AGG / 773 Cairo / 754 minimal Linux Release checks
have passing evidence (AGG/Cairo ELF permission repairs and failed-test reruns;
minimal passed the full run directly). Five restricted heap/global ASan+UBSan
checks pass: both negative-slice tests, both negative-end tests and legacy slice.
Stack ASan and LeakSanitizer remain disabled in this environment.
Signed bound-addition overflow and results above Int32 maximum remain
unsupported; ARRAY_NEGATIVE_SLICE.md / .ru.md record the contract. No stage closes.

v0.374 adds negative-length borrowed push/pop/shift/unshift with signed own
keys and method-specific deletion/length rules. Readonly/nondeletable flags,
raw accessors, Int32 minimum boundaries, insertion budgets, allocation failures,
GC-after-failure and retry behavior are covered in SWF4–8 scenarios. A shared
host-long-sized key buffer removes formatting range-inference warnings.
Validation v0.374: all 771 AGG / 771 Cairo / 752 minimal Linux Release checks
have passing evidence after ELF execution-permission repairs and failed-test
reruns. Five restricted heap/global ASan+UBSan checks pass (stack ASan and LSan
disabled): both new end-operation tests, legacy push/unshift and negative splice.
Native Array negative storage and signed overflow remain unsupported;
ARRAY_NEGATIVE_ENDS.md / .ru.md describe the contract. No stage is closed.

v0.373 adds signed own-key insertion for loaded borrowed splice with negative
captured length. All 769 AGG, 769 Cairo and 750 minimal Linux Release checks have
passing evidence after rebuilding both private-header consumers and repairing
ELF execution permissions with failed-test reruns. Five splice checks pass the
restricted heap/global ASan+UBSan configuration (stack ASan and LSan disabled),
including the new allocation sweep, GC-after-failure and retry checks. Readonly
keys/length, work-budget failure, SWF4–8 zero crossing and conversion side effects
are covered. Native Array negative storage and signed arithmetic overflow remain
unsupported. See ARRAY_NEGATIVE_SPLICE.md / .ru.md. No stage is closed.

v0.372 adds native Try/Catch/Finally using owned child mappings and the shared
With block runner. Suspended baselines, host bindings, returns and exceptions are
rooted; rich resource faults and host/loaded name/register catch rules are retained.
All 767 AGG, 767 Cairo and 748 minimal Linux Release checks have passing evidence
after generated ELF permission repairs and reruns. Eight Try/With/replay checks
pass restricted heap/global ASan+UBSan (stack ASan and LeakSanitizer disabled).
Cold/warm automatic OOM sweeps additionally pass all three profiles and sanitizer.
Fresh fixtures pass all 26 synthetic Unicorn ISA/ABI cases, not a foreign-OS VM
run. The root allocation sweep reproduced and fixed a pre-guard breakpoint-key
allocation escaping the C API. JIT_TRY.md / .ru.md record nested control flow,
64/65-level bounds, source/budget/breakpoint behavior, GC-only saved graphs,
catch-setter failures and remaining conformance/platform gaps. No stage is closed.

v0.371 adds native With with owned compiled child mappings, shared operands/
registers/pools, function-wide Return propagation and scoped lexical cleanup.
All 763 AGG, 763 Cairo and 744 minimal Linux Release checks have passing evidence
after generated ELF permission repairs and reruns, including the minimal reference
runner's child executable. Six With/allocation/callback/validation/definition/
frame-replay checks pass restricted heap/global ASan+UBSan (stack ASan and
LeakSanitizer disabled). Fresh fixtures pass 26 synthetic Unicorn ISA/ABI cases;
these do not execute the complete VM on another OS. Dependent test objects were
explicitly rebuilt for changed private JIT layouts. JIT_WITH.md / .ru.md record
64-level nesting, source/budget boundaries, collecting/throwing callbacks, retained
closures, active child mappings, aggregate cache bytes and breakpoint fallback.
Try, other remaining Actions, reference conformance and real non-Linux runtime
acceptance remain open; no roadmap stage is closed.

v0.370 adds native DefineFunction/DefineFunction2 through shared interpreter/native
record materialization, runtime pool snapshots and rooted closure publication.
All 759 AGG, 759 Cairo and 740 minimal Linux Release checks have passing evidence
after generated ELF permission repairs and reruns. Five definition/allocation/
callback/validation/mode checks pass restricted heap/global ASan+UBSan (stack ASan
and LeakSanitizer disabled). Fresh fixtures pass all 26 synthetic Unicorn ISA/ABI
cases; this is not a foreign-OS VM run. JIT_DEFINITIONS.md / .ru.md record both
record forms, retained lexical/pool captures after parent-code release and GC,
Function2 preload/register bounds, throwing setters, malformed source and atomic
compile failures. With/Try, full Flash conformance and actual non-Linux runtime
acceptance remain open; no roadmap stage is closed.

v0.369 retains bounded stored-frame framing during opt-in interpreter replay.
The regression first failed on v0.368 for an unloaded implicit-End frame. It now
covers host/SWF4–8, explicit/implicit End, replay enabled/disabled, successful and
throwing retries, restored original exception/budget/roots and inactive cached code.
All 755 AGG, 755 Cairo and 736 minimal Linux Release checks have passing
evidence after generated ELF permission repairs and reruns. Four frame/root/
allocation/dispatch checks pass restricted heap/global ASan+UBSan (stack ASan
and LeakSanitizer disabled). All roadmap stages and foreign-platform acceptance
remain open.

v0.368 adds native GetURL/GetURL2/Call and automatic JIT selection for validated
stored frame blocks, with named-breakpoint exclusion. All 754 AGG, 754 Cairo and
735 minimal Linux Release checks have passing evidence after generated ELF
permission repairs and reruns. Five dispatch/callback/allocation/frame/invalid
checks pass restricted heap/global ASan+UBSan (stack ASan/LeakSanitizer disabled).
JIT_DISPATCH.md records counted/legacy URLs, sink errors, nested GC, exact native/
interpreter counters, pool isolation, bounded recursion/recovery, breakpoint/cache
selection, implicit-End blocks and shared instruction limits. Tests use local
recording sinks without network/navigation. Full Flash compatibility, remaining
structured native Actions and real foreign-platform acceptance remain open.

v0.367 adds native WaitForFrame/WaitForFrame2 with counted-record conditional
branches, shared availability conversion and uncharged successful early exits.
All 750 AGG, 750 Cairo and 731 minimal Linux Release checks pass after generated
ELF permission repairs before execution. Five wait/callback/allocation/invalid/
mode checks pass restricted heap/global ASan+UBSan (stack ASan/LeakSanitizer
disabled). Fresh fixtures pass all 26 Unicorn synthetic ISA/ABI regression cases;
this is not execution of the complete VM on foreign CPUs/OSes. JIT_WAIT_FRAMES.md
records exact explicit/implicit-End budgets, malformed source targets, missing
inputs, collecting/throwing conversion and compile/invoke allocation failures.
All roadmap stages remain open.

v0.366 adds native CloneSprite/RemoveSprite/StartDrag/EndDrag through shared scene
services. All 747 AGG, 747 Cairo and 728 minimal Linux Release checks have passing
evidence after generated ELF execute-permission repairs and reruns. Three scene
checks pass restricted heap/global ASan+UBSan (stack ASan/LeakSanitizer disabled).
JIT_SCENE.md records ordered collecting/throwing coercion, unpinned detached drag
roots, selected-target repair, deferred motion, EndDrag exception/retry and fresh-
context allocation failures. Full Flash scene semantics, other Actions and real
foreign-platform acceptance remain open; no stage is closed.

v0.365 adds seven native timeline Actions through shared intrinsic frame services.
All 744 AGG, 744 Cairo and 725 minimal Linux Release checks pass after generated
ELF execute-permission repairs before the graphical-profile runs. Four timeline/
allocation/callback/invalid checks pass restricted heap/global ASan+UBSan (stack
ASan/LeakSanitizer disabled). JIT_TIMELINE.md records selected-target behavior,
frame/label/bias policy, collecting/throwing conversion, post-conversion target
changes, malformed records, and compile/invoke allocation failure containment.
No adb/xcrun/Wine, checked cross-compilers or QEMU user runners were found in the
current PATH; no foreign OS/VM run is claimed. Functional gaps and all stages remain open.

v0.364 adds native Enumerate/Enumerate2 with shared scope/path lookup, rooted
name snapshots and bounded operand publication. All 741 AGG, 741 Cairo and 722
minimal Linux Release checks have passing evidence after generated ELF permission
repairs and reruns. Three enumeration checks pass restricted heap/global ASan+UBSan
(stack ASan/LeakSanitizer disabled), including the strengthened nonempty SWF4 clip
case. JIT_ENUMERATE.md records key order/hiding, counted names, collecting/throwing
lookup, allocation failure, retained results and exact stack bounds. Full reference
Flash ordering, remaining Actions and real foreign-platform acceptance remain open.

v0.363 adds native Trace/Throw/GetTime/RandomNumber through shared execution
and exception services. All 738 AGG, 738 Cairo and 719 minimal Linux Release
checks have passing evidence, including reruns after generated ELF permission
repairs. Four effects/allocation/mode/invalid checks pass restricted heap/global
ASan+UBSan (stack ASan and LeakSanitizer disabled). JIT_EFFECTS.md records rooted
collecting/throwing conversion, trace sink failures, exact random state, elapsed
samples, retained exceptions and allocation failure unwinding. The actual
validate_platform.py --profile all --jit-emulator run passes all three profiles
and 26 Unicorn 2.1.4 synthetic ISA/ABI cases per profile after an initial Cairo
permission failure and repair. This does not certify a foreign VM/OS. All stages remain open.

v0.362 adds native GetProperty/SetProperty/TargetPath through shared scene services.
All 736 AGG, 736 Cairo and 717 minimal Linux Release checks have passing evidence
after generated ELF execute-permission repairs. Six display/class checks pass
restricted heap/global ASan+UBSan (stack ASan/LSan disabled). JIT_DISPLAY.md records
versioned targets, ordered collecting/throwing coercion, skipped writes, counted
paths and allocation checks. Complete compatibility and all stages remain open.

v0.361 adds an optional fail-closed platform encoder-emulation gate. The actual
validate_platform.py --profile all --jit-emulator run passes 733 AGG, 733 Cairo
and 714 minimal CTest checks, plus 26 Unicorn 2.1.4 cases per profile. An initial
minimal run correctly failed for two non-executable generated ELF files; their
permissions were restored before the successful run. Five orchestration tests
and explicit optimized-checker rejection pass. No C/C++ runtime code changed;
v0.360 class sanitizer evidence remains applicable. This is native Linux and
synthetic foreign-ISA evidence, not foreign-OS VM acceptance. All stages remain open.

v0.360 adds native Extends/Implements/InstanceOf/Cast through shared class services.
All 732 AGG, 732 Cairo and 713 minimal Linux Release checks have passing evidence
after generated ELF execute-permission repairs; three class tests pass restricted
heap/global ASan+UBSan (stack ASan/LSan disabled). JIT_CLASSES.md records isolated
versioned, snapshot, accessor/boxing, exception and allocation checks. All stages
and real foreign-platform runtime acceptance remain open.

v0.359 adds native legacy Equals/Less and logical And/Or. All 729 AGG, 729 Cairo
and 710 minimal Linux Release checks have passing evidence after generated ELF
execute-permission repairs. Expanded hook cases were rebuilt and pass in all
profiles and restricted heap/global ASan+UBSan; six focused sanitizer checks pass
(stack ASan/LSan disabled). Unicorn 2.1.4 also passes all 26 current foreign-ISA
synthetic ABI/control-flow cases. These are encoder/stub-helper executions, not
full foreign-platform VM acceptance. See JIT_LEGACY.md and JIT_ARCHITECTURES.md;
all nine stages remain open.

v0.358 adds native InitArray/InitObject using shared count/construction services.
All 726 AGG, 726 Cairo and 707 minimal Linux Release checks have passing evidence
after restoring generated ELF execute permissions. Three literal checks pass
restricted heap/global ASan+UBSan (stack ASan/LSan disabled). JIT_LITERALS.md
records versioned count/entry/GC/allocation behavior. All stages remain open.

v0.357 adds twelve native string Actions using shared counted/versioned services.
All 723 AGG, 723 Cairo and 704 minimal Linux Release checks have passing evidence
after restoring generated ELF execute permissions. Three string checks and the
updated fallback fixture pass restricted heap/global ASan+UBSan (stack ASan/LSan
disabled). JIT_STRINGS.md records preserved SWF4–5/character limits. All stages
and foreign-platform runtime acceptance remain open.

v0.356 adds native SetTarget/SetTarget2 using shared timeline selection. All 720
AGG, 720 Cairo and 701 minimal Linux Release checks have passing evidence after
restoring generated ELF execute permissions. The latest target fixtures were
explicitly rebuilt and pass in all profiles and restricted heap/global ASan+UBSan
(stack ASan/LSan disabled). See JIT_TARGETS.md. All nine stages remain open.

v0.355 adds native CallFunction/NewObject and versioned lexical/target lookup.
All 718 AGG, 718 Cairo and 699 minimal Linux Release checks have passing evidence;
failed launches required restoring generated ELF execute permissions. Three new
checks pass restricted heap/global ASan+UBSan, with stack ASan and LSan disabled.
See JIT_NAMED_CALLS.md. Other platforms, complete AVM1 and all stages remain open.

v0.354 adds native CallMethod/NewMethod via shared AVM1 dispatch. All 715 AGG,
715 Cairo and 696 minimal Release checks have passing evidence after restoring
generated ELF execute permissions and rebuilding the corrected method fixture.
Both new tests pass restricted heap/global ASan+UBSan; stack ASan and LSan remain
disabled. JIT_METHODS.md records scope. All nine stages remain open.

v0.353 adds native ConstantPool and indexed Push with captured activation pools.
Linux Release has passing evidence for all 713 AGG, 713 Cairo and 694 minimal
checks. AGG/Cairo test objects were rebuilt after the private context layout
changed; AGG and minimal required restoring generated ELF execute permissions.
Ten focused restricted ASan+UBSan checks pass. See JIT_POOLS.md; all roadmap
stages and foreign-platform runtime acceptance remain open.

v0.352 adds native register reads/stores and matching prepared/root bank lifetimes.
Linux Release has passing evidence for 709 AGG, 709 Cairo and 690 minimal tests,
after documented generated-artifact repairs; nine focused restricted ASan+UBSan
checks pass. JIT_REGISTERS.md records exact scope and limits. This is not complete
AVM1/JIT or non-Linux acceptance, and all stages remain open.

v0.351 adds native numeric literals/unary operations and shared interpreter IEEE
decoding. Final full Linux Release runs pass directly: 706 AGG, 706 Cairo and 687
minimal tests; nine restricted ASan+UBSan checks also pass. See JIT_NUMBERS.md for
intermediate permission repairs and the updated fallback fixture. Real foreign
target/OS acceptance and the remaining roadmap requirements are still open.

v0.350 extends the native reference subset. Linux Release has passing evidence
for all 704 AGG, 704 Cairo and 685 minimal checks, with documented ELF permission
repairs, plus six restricted ASan/UBSan checks. See JIT_REFERENCES.md. This does not
close stages 8/9 or the other compatibility gates.

Текущая инфраструктура проверки платформ описана в
[PLATFORM_VALIDATION.ru.md](PLATFORM_VALIDATION.ru.md). Исторические контрольные
точки ниже не отменяют JIT v0.346–0.348 и действующий запрет выдачи архивов.
Новые инструменты не заменяют реальные запуски Windows/macOS/Android.

The current native/JIT evidence is in JIT_ARCHITECTURES.md; repeatable platform
runs are in [PLATFORM_VALIDATION.md](PLATFORM_VALIDATION.md). Older checkpoints
below describe their own versions. The standing instruction disables source/Git
archive delivery. All nine stages remain open.

v0.349 Linux Release evidence: AGG 701/701, Cairo 701/701, minimal 682/682,
with examples/reference runner. Generated-ELF permission repairs were needed
before the final full passing runs; see PLATFORM_VALIDATION.md for scope and
failure-path checks. Windows/macOS/Android have no new runtime evidence.

| Стадия | Проверенная часть | Что требуется для завершения |
| --- | --- | --- |
| 1 | Counted UTF-16 values/property/member/global names, selected Object/Array/Number semantics | Full arguments/type conformance, Array size boundaries, full API inventory and historical/versioned conversion comparisons |
| 2 | Selected Function2/preloads/arguments, inheritance/super, numeric and member/variable actions | Complete action/version/scope/exception semantics with compiler-generated SWFs and a reference oracle |
| 3 | Frame actions/calls, sprites/exports, selected scene controls, target scopes/paths, indexed fields, shapes/bitmaps and clip APIs | Complete SWF display list/resources/masks/drag/load/text creation and event ordering |
| 4 | Selected raster/bitmap/filter/geometry/text operations and bundled dependencies | Complete text/font integration, filter/blend semantics, audio/video/streaming and historical comparisons |
| 5 | Selected standard classes and host input/network/URL request bridges | Complete AS1/AS2 inventory, XML/XMLSocket/LoadVars/SharedObject, loaders/timers/events/system APIs |
| 6 | Selected graph/resource/worker/channel/mutex transport, nested fault context | Distributed failures/ownership reconciliation and deadlocks, all built-in graph cases and extension-module acceptance |
| 7 | Selected GC roots/cycles/native captures/finalizers, freeze/peek/local wait cycles | All-subsystem fault/stress acceptance, full distributed debugger graphs and complete memory-tool evidence |
| 8 | Linux x64 native subset and runtime modes; five additional Linux ABI encoders checked with synthetic helpers in Unicorn | Complete native semantics, real foreign-CPU/OS execution, GC/debug/exception and interpreter-equivalence acceptance on each target |
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

The JIT build-default preset, media and unsupported presentation keys still fail
configuration; runtime JIT modes do exist. A platform key, source adapter, cross-compile
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

### v0.304 — missing variable/target operands / Имена переменных и целей

Six loaded variable/declaration/target actions now supply undefined on missing
operands; host name/stack checks stay strict. Raw SWF4–8 fixtures observe live
SetTarget2 reset/invalidation and caller restoration, plus named scope operations.
See AVM1_VARIABLE_TARGET_MISSING.md. Full scope/target semantics remain open.

### v0.305 — missing display/URL/With operands / Свойства, URL и With

Four loaded actions prepare missing undefined values while retaining payload
validation order and host strictness. Raw SWF4–8 tests distinguish target paths,
URL sink arguments and skipped With bodies. Display/URL layout constants are
named; an unused URL root is removed. See AVM1_DISPLAY_URL_WITH_MISSING.md.
Полные семантики и все стадии роудмапа остаются открытыми.

### v0.306 — shared frame-request preparation / Общая подготовка кадра

GotoFrame2 conversion now uses the C89 fvm_exec_frame_request service with rooted
coercion, alias/error preservation and separate scene-bias policy. Timeline wire
widths/root layouts use named macros. Raw Call/GotoFrame2 underflow fixtures cover
SWF4–8 alongside direct C, collecting/throwing callbacks and allocation injection.
See FRAME_REQUEST_SERVICE.md. Complete frame semantics and all stages remain open.

### v0.307 — missing class operands / Недостающие операнды классов

Loaded Extends/CastOp/InstanceOf supply missing undefined values with preserved
operand roles. Raw host/SWF4–8 fixtures verify primitive boxing, superclass getter
GC/throws and root/execution restoration. C services, interface counts and ABI
remain unchanged. See AVM1_CLASS_MISSING.md. All stages remain open.

### v0.308 — missing ImplementsOp operands / ImplementsOp без фиксированных значений

Loaded missing count/constructor values become undefined; zero count preserves
prototype snapshots and does not box primitive constructors. Raw host/SWF4–8 and
allocation-failure fixtures verify continuation, root restoration and no Number
callback. See AVM1_IMPLEMENTS_MISSING.md. Historical policy gaps/all stages stay open.

### v0.309 — missing call operands / Недостающие операнды вызова

Four loaded call actions prepare undefined fixed operands with preserved operand
roles, name/count coercion order and nullish receiver policy. Raw host/SWF4–8
fixtures cover zero arguments, collecting/throwing bodies and allocation failures.
Shared C services and ABI remain unchanged. See AVM1_CALL_MISSING.md.
All roadmap stages remain open.

### v0.310 — empty StoreRegister / StoreRegister с пустым стеком

Loaded missing values initialize undefined before existing bank selection;
host strictness and malformed-payload ordering remain intact. The register
selector width has a documented semantic macro. Raw global/private Function2
and allocation-failure coverage is recorded in AVM1_STORE_MISSING.md.
All roadmap stages remain open.

### v0.311 — missing pool entries / Отсутствующие строки таблицы

Loaded Constant8/16 misses produce undefined and continue; malformed index fields
and strict host behavior retain their errors. Separate documented macros describe
the wire widths. Raw returned-value/continuation and allocation fixtures are
recorded in AVM1_CONSTANT_MISSING.md. All roadmap stages remain open.

### v0.312 — native detach identity / Признак отсоединённого клипа

Native removal and depth replacement commit an explicit private marker together
with parent/playback state. A value-based C89 query bypasses script overrides and
handles older state. Atomic removal failure checks include the marker. See
CLIP_DETACH_IDENTITY.md. Descendant/unload/Action policy and all stages stay open.

### v0.313 — dynamic removal policy / Правила удаления динамического клипа

A shared C89 service filters intrinsic depth, parent and detached ancestry before
atomic native removal. Script overrides cannot replace the policy. Raw C and
allocation fixtures cover supported depth boundaries, roots and detached subtrees.
See DYNAMIC_CLIP_REMOVAL_SERVICE.md. Action/lifecycle gaps and all stages stay open.

### v0.314 — RemoveSprite native dispatch / Удаление клипа инструкцией

A common explicit-value C89 service resolves native/path targets and applies
intrinsic dynamic removal. The interpreter handles missing values and target
fallback; a shared ancestor helper supplies effective detachment. Raw/C/GC/throw
and allocation fixtures are documented in AVM1_REMOVE_SPRITE.md. Dispatch audit
is now 95/100; lifecycle gaps and every roadmap stage remain open.

### v0.315 — retained AVM1 ancestry / Родство удалённого клипа

Loaded removal/replacement commits a historical parent edge alongside physical
detachment. Intrinsic parent/root/path reads share a versioned helper. Host behavior
stays parentless. GC, channel remapping and allocation checks are documented in
MOVIECLIP_REMOVED_ANCESTRY.md. All stages and full unload/lifetime policy stay open.

### v0.316 — removed target selection / Выбор удалённой цели

The shared loaded selection service filters detached native bases/descendants
before publishing a target. Host behavior and initial activation construction
remain unchanged. Raw activation probes and allocation checks are recorded in
AVM1_REMOVED_TARGET_SELECTION.md. Full lifetime policy and all stages stay open.

### v0.317 — native root locks / Блокировка корня

A common C89 service atomically controls native _lockroot with versioned Boolean
conversion. Root/preload/absolute-path traversal honors the flag; parent edges
remain unchanged. Stable accessor IDs support graph transport. Fresh recovered
AGG/Cairo builds passed 616 tests each; three limited sanitizer fixtures and four
recovered original SWFs passed in both profiles. See MOVIECLIP_LOCKROOT.md for
exact coverage and the pruned historical corpus limitation. All stages stay open.

### v0.318 — root-relative hit points / Координаты hitTest

Loaded point tests transform from unlocked root coordinates after argument
coercion and use versioned shape-flag truth. Host world-coordinate policy and
object overlap remain unchanged. C89, moving/throwing coercion and allocation
fixtures are recorded in MOVIECLIP_HIT_ROOT_COORDINATES.md. All stages stay open.

## v0.319 explicit clip resolution / Явное разрешение клипа

Linux GCC/G++ 13.3 Release: AGG 622/622 and Cairo 622/622 CTest checks pass.
The three new hit-target tests pass in the restricted headless heap/global
ASan+UBSan profile (stack instrumentation and LeakSanitizer disabled).
Four unmodified pinned Ruffle originals match stdout exactly, exit zero and
produce no stderr in both Release profiles. This reruns four recovered fixtures,
not the former 26-fixture corpus. Target-removal regression checks also pass.
See MOVIECLIP_HIT_TARGET.md for behavior and limitations; all stages remain open.

Linux: AGG и Cairo — по 622/622; три новых теста под ограниченными ASan/UBSan;
по четыре исходных SWF с точным выводом. Полная совместимость не заявляется.

## v0.320 shared native hit geometry / Общая геометрия hitTest

Linux GCC/G++ 13.3 Release AGG and Cairo each pass 625/625 CTest checks.
Five focused checks pass in the restricted headless heap/global ASan+UBSan
profile: C hit geometry, allocation failures, geometry callbacks, loaded root
callbacks and loaded target callbacks. Stack ASan and LeakSanitizer remain
disabled because of the previously probed runtime limitation.
Four recovered unmodified pinned Ruffle originals again match exact stdout,
exit zero and have empty stderr in both Release builds. This is a four-fixture
rerun, not a rerun of the former 26-fixture corpus. All stages remain open.
See CLIP_HIT_GEOMETRY_SERVICE.md for the C interface and geometry limitations.

Linux: по 625/625 тестов AGG/Cairo, пять целевых проверок ограниченных ASan/UBSan
и по четыре исходных SWF. Проверены прямые C-вызовы, подмена скриптовых методов,
геттеры с исключениями, запрет вложенного GC и сохранение выхода при отказах.
Полная геометрическая совместимость, JIT и приёмка других ОС остаются открытыми.

## v0.321 intrinsic sibling construction / Общее дублирование

Linux GCC/G++ 13.3 Release AGG and Cairo each pass 628/628 CTest checks.
Three direct duplication checks pass in the restricted heap/global ASan+UBSan
profile; stack ASan and LeakSanitizer remain disabled. Four recovered pinned
unmodified SWFs again match stdout exactly with exit zero and empty stderr in
both Release builds. See DUPLICATE_CLIP_SERVICE.md for the interface, unchanged
native limitations and the absence of a scene-rollback promise.

Linux: по 628/628 AGG/Cairo, три теста ограниченных ASan/UBSan и по четыре
исходных SWF. Все стадии роудмапа остаются открытыми.

## v0.322 CloneSprite / Общий запрос дублирования

Linux GCC/G++ 13.3 Release: AGG 632/632 and Cairo 632/632 CTest checks pass.
Four focused clone tests pass under the restricted heap/global ASan+UBSan
profile, including every direct/interpreter allocation boundary and collecting
coercions. Stack instrumentation and LeakSanitizer remain disabled.
Four recovered unmodified pinned originals again match exact stdout, exit zero
and empty stderr in both Release builds. No SWF15 fixture is relabeled or counted
as SWF≤8 acceptance. Source references now cover 96/100 catalogued Action codes;
--require-all still correctly fails. See AVM1_CLONE_SPRITE.md for native limits.

Linux: по 632/632 AGG/Cairo, четыре целевых теста ограниченных ASan/UBSan и по
четыре исходных SWF. Реестр диспетчеризации — 96/100 ссылок на ветки, без заявления
полной семантики. Все стадии открыты; JIT и остальные ОС не объявлены готовыми.

## v0.323 input validation / Единая проверка ввода

Linux GCC/G++ 13.3 Release AGG passes 633/633 CTest checks. Cairo has 632 passes
and one BAD_COMMAND launch failure: the existing avm1_target_api executable lost
its execute bit. After restoring that exact ELF's mode, its focused rerun passes,
so all 633 checks have passing evidence. No behavior failure was hidden by retry.
Four input/queue/gesture checks pass in the restricted heap/global ASan+UBSan
profile (stack ASan and LeakSanitizer disabled). Four recovered original SWFs
match exact stdout, exit zero and empty stderr in each Release profile.

Linux: 633/633 AGG; все 633 Cairo прошли с повторным запуском одного существующего
теста после восстановления права исполнения ELF. Четыре целевых теста
ограниченных ASan/UBSan и по четыре исходных SWF. См. INPUT_VALIDATION.md.
Все стадии остаются открытыми.

## v0.324 callback failure recovery / Восстановление после исключений ввода

Linux GCC/G++ 13.3 Release Cairo passes 634/634 CTest checks directly. AGG has
633 passes and one BAD_COMMAND launch failure for bitmap_noise_allocation; after
restoring that exact ELF's execute bit, its focused rerun passes. Thus all 634
checks have passing evidence in each profile, without retrying behavior failures.
Five focused input tests pass in the restricted heap/global ASan+UBSan profile
(stack ASan and LeakSanitizer disabled), including GC inside throwing callbacks.
Four recovered unmodified pinned SWFs match exact stdout, exit zero and empty
stderr in each Release build. See INPUT_CALLBACK_FAILURES.md; all stages open.

Linux: 634/634 Cairo; все 634 AGG прошли с повторным запуском одного теста после
восстановления права исполнения ELF. Пять тестов ограниченных ASan/UBSan и по
четыре исходных SWF. Проверены исключения host callbacks и восстановление очереди;
полная поддержка Flash, JIT и приёмка остальных ОС остаются открытыми.

## v0.325 construction decomposition / Декомпозиция создания клипов

Linux GCC/G++ 13.3 Release AGG and Cairo each pass 634/634 CTest checks directly.
Ten focused native/script duplication, CloneSprite, gradient/bitmap allocation
and transport checks pass under the restricted heap/global ASan+UBSan profile;
stack ASan and LeakSanitizer remain disabled. The two moved helper bodies match
the previous source token-for-token after symbol renaming. Four recovered
unmodified pinned original SWFs match exact stdout, exit zero and empty stderr
in both profiles. See MOVIECLIP_CONSTRUCTION_DECOMPOSITION.md. All stages open.

Linux: по 634/634 AGG/Cairo без повторных запусков, десять целевых проверок
ограниченных ASan/UBSan и по четыре исходных SWF. Перенос функций подтверждён
сравнением токенов и существующими сценариями; алгоритмы и публичный ABI сохранены.

## v0.326 world pointer state / Мировая позиция указателя

Linux GCC/G++ 13.3 Release Cairo passes 635/635 CTest checks directly. AGG has
634 passes and one BAD_COMMAND launch failure for the existing
execution_removed_target_selection_allocation ELF. Restoring its execute bit
and rerunning that exact test succeeds, giving all 635 passing checks.
Six input checks pass in the restricted heap/global ASan+UBSan profile (stack
ASan and LeakSanitizer disabled). Four recovered unmodified pinned original SWFs
match exact stdout, exit zero and empty stderr in each profile. See INPUT_POINTER.md.

Linux: все 635 проверок каждой сборки прошли; AGG потребовал восстановления
права исполнения одного существующего ELF и его повторного запуска. Шесть
проверок ограниченных ASan/UBSan и по четыре исходных SWF. Все стадии открыты.

## v0.327 local pointer coordinates / Локальные координаты указателя

Linux GCC/G++ 13.3 Release AGG and Cairo each pass 639/639 CTest checks directly.
Four new pointer-query/raw-AVM1 tests pass under restricted heap/global ASan+UBSan
(stack ASan and LeakSanitizer disabled). Four recovered unmodified pinned SWFs
match exact stdout, exit zero and empty stderr in both profiles. Tests cover
transformed ancestors, destination-context pointer access after transport, getter
mutation/errors, legacy readonly policy and every query allocation boundary.
See CLIP_POINTER.md; no full input/player-version equivalence or JIT is claimed.

Linux: по 639/639 AGG/Cairo, четыре проверки ограниченных ASan/UBSan и по четыре
исходных SWF. Все стадии открыты; полный drag и приёмка других ОС ещё не выполнены.

## v0.328 native drag engine / Нативное перетаскивание

Linux GCC/G++ 13.3 Release: all 642 CTest checks have passing evidence in AGG
and Cairo. Initial runs had only BAD_COMMAND launch failures: two AGG and six
Cairo existing ELF files lost execute permission. Restoring those exact files
and rerunning only those tests passes; no behavior failure was retried away.
Five focused drag/input tests pass under restricted heap/global ASan+UBSan
(stack ASan and LeakSanitizer disabled). Four recovered unmodified pinned SWFs
match exact stdout, exit zero and empty stderr in both profiles.
See NATIVE_DRAG_SERVICE.md for C-only scope and remaining compatibility work.

Linux: все 642 проверки каждой сборки прошли с повторным запуском двух AGG и
шести Cairo тестов после восстановления прав исполнения ELF. Пять проверок
ограниченных ASan/UBSan и по четыре исходных SWF. Drag пока подключён из C и
host input; Action/скриптовые адаптеры ещё открыты. Все стадии открыты.

## v0.329 StartDrag / EndDrag

Linux GCC/G++ 13.3 Release AGG and Cairo each pass 645/645 CTest checks directly.
Three raw Action/coercion/GC/allocation checks pass in restricted heap/global
ASan+UBSan (stack ASan and LeakSanitizer disabled). Four recovered original SWFs
match exact stdout, exit zero and empty stderr in both profiles. Source Action
references now cover 98/100 entries, with ToggleQuality/StopSounds absent;
--require-all remains an expected failure. See AVM1_DRAG.md for supported scope.

Linux: по 645/645 AGG/Cairo, три теста ограниченных ASan/UBSan и по четыре
исходных SWF. Проверка ссылок на Action-ветки — 98/100, без заявления полной
совместимости; все стадии открыты.

## v0.330 MovieClip drag methods / Скриптовые методы drag

Linux GCC/G++ 13.3 Release AGG and Cairo each pass 648/648 CTest checks directly.
Three new C89/native option, callback/GC and allocation checks pass in restricted
heap/global ASan+UBSan (stack ASan and LeakSanitizer disabled). Four recovered
unmodified pinned SWFs match exact stdout, exit zero and empty stderr in both
Release profiles. Channel transport preserves appended callback IDs and uses
independent destination pointer/drag state. Source Action references remain
98/100; only ToggleQuality/StopSounds lack dispatch. See MOVIECLIP_DRAG.md.

Linux: по 648/648 AGG/Cairo, три новых теста ограниченных ASan/UBSan и по четыре
исходных SWF. Проверены C и нативные методы, GC в преобразованиях, частичные
аргументы, канал, исключения и отказы памяти. Все стадии роудмапа остаются открытыми.


v0.373: borrowed loaded negative-length splice uses signed own insertion keys,
captured conversion bounds and rooted empty removals; native negative storage
and signed overflow remain unsupported. Этапы остаются открытыми.
See [EN](ARRAY_NEGATIVE_SPLICE.md) / [RU](ARRAY_NEGATIVE_SPLICE.ru.md).


v0.386: loaded String range copies charge each emitted UTF-16 unit with shared
nested work, saturation/error atomicity and missing-argument noops.
RU: учёт копирования slice/substring/substr; все девять стадий открыты.
Validation v0.386: 39 AGG String CTest checks pass. New range-copy and existing
loaded-work checks pass in Cairo/minimal Release and restricted heap/global
ASan+UBSan (stack ASan and LeakSanitizer disabled). These are targeted Linux
checks; no new full-profile, external-oracle or foreign-platform claim.


v0.387: concat/fromCharCode charge argument visits and UTF-16 output, sharing
work with callbacks. RU: общий бюджет построения строк; все стадии открыты.
Validation v0.387: 41 AGG String CTest checks pass. Builder, builder-callback and
range-copy checks also pass in Cairo/minimal Release and restricted heap/global
ASan+UBSan; stack ASan and LeakSanitizer disabled. No full-profile, new external
oracle or non-Linux execution is claimed.


v0.388: public loaded String casing charges UCS2 mapping and UTF-16 output
with shared work; internal property folding keeps its caller policy.
RU: учёт публичного изменения регистра; все девять стадий открыты.
Validation v0.388: all 43 String executables are rebuilt against the current
static library and pass in Linux AGG/Cairo/minimal Release. Six rebuilt checks
pass in restricted heap/global ASan+UBSan: range copies, builders, callbacks,
casing, interpreted/explicit/automatic native copies and existing loaded-work
allocation failures. Stack ASan and LeakSanitizer remain disabled. Targeted
checks do not certify full compatibility; all nine stages remain open.


v0.389: loaded charAt/charCodeAt share receiver/index callback work and charge
one extracted UTF-16 unit. RU: общий бюджет извлечения символа; стадии открыты.
Validation v0.389: all 44 String executables are rebuilt and pass in Linux
AGG/Cairo/minimal Release. Seven rebuilt restricted heap/global ASan+UBSan
checks pass, adding character callback/budget extraction to the six v0.388
checks. Stack ASan and LeakSanitizer remain disabled. No full-suite, original
external-oracle or foreign-platform acceptance is claimed for this checkpoint.


v0.390: independent traced legacy/exact registerClass maps survive reload, GC,
channel transport and replacement OOM. Старые плоские записи сохраняют прежнюю
политику; полное разделение globals/base-clip provenance открыто. Все стадии открыты.
Validation v0.390: nine rebuilt registry/base checks pass in Linux Release
AGG/Cairo/minimal and in restricted heap/global ASan+UBSan (stack ASan and
LeakSanitizer disabled). Existing transfer-name checks now explicitly activate
the sender's version environment after verifying legacy entries are hidden in
the unloaded destination. The new constants header passes strict C89 syntax.


v0.391: retained source-movie metadata selects registerClass environment from
the activation base clip, independently of SetTarget/ambient version.
RU: версия базового клипа для регистрации; старые графы используют fallback. Стадии открыты.
Validation v0.391: fourteen rebuilt registry, base and scene checks pass in
Linux Release AGG/Cairo/minimal and restricted heap/global ASan+UBSan, including
source-version callbacks/transport, replacement OOM, sprite-load OOM and native
timeline allocation. Stack ASan and LeakSanitizer disabled. The new source
metadata constants header passes strict C89 syntax. All stages remain open.


v0.392: attach/duplicate/authored placement choose registered constructors by
source symbol version, including transported fallback maps.
RU: версия символа при создании экземпляров; старые библиотеки используют fallback. Стадии открыты.
Validation v0.392: 36 rebuilt AGG registry/scene/timeline/duplicate checks pass;
17 rebuilt registry/scene/native checks pass in Cairo and minimal Release and
restricted heap/global ASan+UBSan. This includes three new source-provenance,
attach-allocation and interpreted/explicit/automatic registration cases. Stack
ASan and LeakSanitizer disabled. Source comparison is not a Flash Player oracle;
full-profile and non-Linux acceptance remain open, as do all roadmap stages.


v0.393: counted TextField storage preserves embedded NUL through layout, UTF-16
range edits, GC and binary/queue graph transport. RU: хранение текста с NUL.
Strict scalar text and native size bounds are retained; full TextField compatibility remains open.
Validation v0.393: nine rebuilt text/graph checks pass in Linux Release
AGG/Cairo/minimal and restricted heap/global ASan+UBSan. Font-disabled checks
retain unavailable-font behavior; font-enabled cases cover counted NUL layout,
editing, OOM/GC and binary/queue round trips. Stack ASan and LeakSanitizer
remain disabled. The new text constants header passes strict C89 syntax.


v0.394: TextFormat validates complete counted alignment/font/URL/target values.
RU: скрытый NUL не обходит ограничения форматирования; стадии остаются открытыми.
Validation v0.394: ten rebuilt text/graph checks pass in Linux Release
AGG/Cairo/minimal and restricted heap/global ASan+UBSan. Font-enabled checks
exercise full counted formatting validation; disabled-font configurations retain
stub behavior. Stack ASan and LeakSanitizer disabled. All stages remain open.


v0.395: TextField graph restore enforces native byte/scalar storage bounds
before native/font allocation. RU: проверка повреждённых графов и отката. Стадии открыты.
Validation v0.395: eleven rebuilt text/graph checks pass in Linux Release
AGG/Cairo/minimal and restricted heap/global ASan+UBSan. New font-enabled cases
reject CRC-valid oversized native payloads with rollback and accept exact scalar,
byte and UTF-16 boundaries. Font-disabled behavior remains unchanged; stack
ASan and LeakSanitizer disabled. All stages remain open.


v0.396: identical counted loaded TextField text assignment preserves current
format/layout; changed text applies future format. RU: повторное присваивание
сохраняет формат; host-политика прежняя. Стадии остаются открытыми.
Validation v0.396: the full rebuilt Linux AGG Release suite passes 826/826 CTest
checks directly. Sixteen focused AGG text/graph/layout checks and thirteen each
in Cairo/minimal Release and restricted heap/global ASan+UBSan also pass.
Font-disabled checks retain unavailable-font behavior. New loaded assignment
cases cover empty/ordinary/NUL content, current/future formats, aliasing, GC and
interpreted/explicit/automatic native SetMember/GetMember. Stack ASan and
LeakSanitizer remain disabled; no foreign-platform or Flash Player oracle claim.
All nine roadmap stages remain open.

v0.397: opt-in live function interfaces call original C/AVM1 functions on the
broker thread through the existing listener, with target/source/context/call
correlation, returned values, counted exceptions and complete diagnostic fields.
RU: интерфейсы оригинальных функций, включая C без кодека; эксклюзивная передача
контекста потоку брокера. See REMOTE_FUNCTIONS.md / .ru.md for ownership/limits.
Validation: full rebuilt Linux AGG Release CTest passes 829/829. Five focused
checks each pass in AGG/Cairo/minimal Release and restricted heap/global
ASan+UBSan with platform sockets enabled. This includes 100 calls from four
independent clients, original capture roots, script globals, reply ID rejection,
forwarding, stale targets and stop/source cleanup. Strict C89 public/protocol
header syntax passes. Stack ASan and LeakSanitizer disabled. Persistent socket
multiplexing, reentrant callback cycles and foreign-platform runtime acceptance
remain open; all nine roadmap stages remain open.

v0.398: function replies require value graphs for success/exception and reject
contradictory successful fault flags. Request flags are validated before dispatch.
RU: строгие ответы, циклы аргументов и возврат интерфейса C-функции.
Six focused checks pass in Linux AGG/Cairo/minimal Release and restricted
heap/global ASan+UBSan with sockets enabled. Expanded fake-peer protocol cases
also pass in all four configurations: four wrong routing IDs, absent values,
contradictory/unknown flags, invalid status and malformed diagnostic magic/text.
The graph case preserves receiver/argument aliases and cycles while isolating
copies, returns a codec-free C interface and rejects calls after source reclaim.
Stack ASan and LeakSanitizer disabled; all roadmap stages remain open.

v0.399: finish new function target URI allocation before publishing/pinning the
export. Failed C++ allocations leave no entry/root; canonical re-export pins
once. RU: отказ выделения URI не оставляет цель/корень. Public malloc-owned URI
copy failure and earlier exports of a failing whole graph are not rolled back.
Eight focused checks pass in Linux AGG/Cairo/minimal Release and restricted
heap/global ASan+UBSan with sockets enabled. The new prepared-body case proves
original attachment execution, self-unbinding, active capture/argument roots
through GC and subsequent original-bytecode execution. C89 public header syntax
passes. Full rebuilt AGG execution passes 830 checks directly; two additional
JIT checks pass individually after executable-mode repair, but both full CTest
attempts encounter recurring permission-denied startup for these same binaries.
This is not recorded as a clean 832/832 full CTest run. Stack ASan and
LeakSanitizer disabled; all nine stages and foreign-platform acceptance remain open.

v0.400: same-authority nested function interfaces dispatch on the existing owner
thread while retaining copied argument/result graphs and original heap leases.
A shared depth bound of 32 prevents unbounded direct recursion; success, status
failure, exceptions and bad_alloc all release their active-depth leases.
RU: вложенные вызовы A→B→A без ожидания собственного слушателя.
Validation: fresh Linux minimal Release passes the full 813/813 CTest suite.
Nine focused broker/function checks pass in minimal and AGG Release and in
restricted heap/global ASan+UBSan. New cases cover two source heaps, return to an
active original heap, bounded recursion, counted exception, diagnostic and OOM
propagation, reclaim/re-enable and recovery. Strict C89 public/protocol headers
pass. Stack ASan and LeakSanitizer disabled; restricted runner links explicit
sanitizer defaults because environment-only options do not suppress unavailable
/proc task enumeration. No cross-authority callback cycle, resource reentrancy
or non-Linux runtime certification; all nine roadmap stages remain open.

v0.401: a documented C89 function_rpc tutorial serves an unregistered original C
callback to an independent client process. Server-owned userdata observes four
attempts; client verifies 42, counted NUL exception, unsupported and plain error.
A Python native-process harness bounds startup/call/shutdown waits and cleans up
children. No callback codec or process-local pointer is transferred.
RU: оригинальная C-функция вызывается из другого процесса через один URI/порт.
Validation: full Linux minimal Release CTest passes 815/815. Eleven focused
broker/function/example checks pass in minimal and AGG Release and restricted
heap/global ASan+UBSan, including the actual two-process exchange. Example passes
strict C89 syntax both with OS_PORTS and with its disabled stub. Stack ASan and
LeakSanitizer disabled. Cross-compiling does not register the host subprocess
harness; Windows/macOS/Android runtime acceptance and all stages remain open.

v0.402: the function_rpc tutorial supports separate processes, two independent
VM runtimes on separate execution threads, and configurable LAN listening/
advertised IPv4 or DNS host/port. Thread mode uses public Worker-owned runtime
and copied primitive URI, then actual TCP invocation into the original VM.
RU: три схемы с командами в FUNCTION_RPC_EXAMPLES.md / .ru.md.
Validation: full Linux minimal Release CTest passes 816/816. Usage, independent
VM-thread and process/LAN-configuration checks pass in minimal and AGG Release
and restricted heap/global ASan+UBSan. Process cases use default loopback,
wildcard/interface binds, advertised IPv4/DNS and reject malformed ports before
startup. Updated example passes strict C89 enabled/stub syntax. Stack ASan and
LeakSanitizer disabled. Tests are local: no two-physical-computer LAN execution
or Windows/macOS/Android runtime certification is claimed. All stages remain open.

v0.403: individually revoke an original function after reclaiming its source.
Drop only its authority root, reject old interfaces, and generate a fresh target
on re-export while preserving other owners and host pins. Retain fixed-width
broker tokens inline so vector entry cleanup allocates no temporary strings.
RU: отзыв функции и удаление смешанных записей брокера без выделений памяти.
Validation: full Linux minimal Release CTest passes 817/817. Thirteen focused
broker/function/process/thread checks pass in AGG Release and restricted heap/
global ASan+UBSan; the full minimal run includes them. New all-C++-allocations-
denied case covers serving rejection, revoke/missing revoke, fresh vs stale
URI, two heaps' independent pins, context detachment and broker stop with mixed
channel/function entries. Two disabled-OS-port checks pass in a fresh no-OS build.
Public/new private constants pass strict C89 syntax. Stack ASan and LeakSanitizer
disabled; physical LAN and non-Linux runtime validation remain open, as do all stages.

v0.404: add standalone EN/RU example README files and link them from the
embedding index. RU: назначение, требования, команды сборки/запуска/CTest
находятся непосредственно в function_rpc. Documentation-only change; relative
links and target/test names checked against source/CMake. No new runtime test
result or stage completion claim.

v0.405: retain mutex peer nonce/context keys inline so stop does not allocate
strings while clearing destroyed client contexts. RU: остановка при нехватке
памяти освобождает два удерживаемых удалённых мутекса и сохраняет корни функций.
The new broker_peer_stop_allocation reproduced bad_alloc before the change;
all-new-denied stop and subsequent source detachment now pass with two peer
contexts and a live function export. Seven focused checks pass in AGG Release
and restricted heap/global ASan+UBSan. Linux minimal passes 818/818 CTest checks;
nine unchanged binaries require a generated, untracked mode-restoring launcher
because the runner resets executable permission. Assertions/arguments are
unchanged; the initial direct run passed 809 and could not start those nine.
The expanded private constants pass strict C89 syntax. Stack ASan and
LeakSanitizer disabled; no foreign OS/physical LAN claim and all stages remain open.

v0.406: broker startup catches member-constructor failures within the public C
boundary, returns OUT_OF_MEMORY/null for allocation failure and preserves explicit
status failures while releasing initialized resources. RU: исключения строковых
конструкторов не выходят из C API. broker_start_allocation reproduced escaped
bad_alloc before the change; the allocation sweep, occupied-port failure and
reuse after stop now pass. Eight focused checks pass in AGG Release and restricted
heap/global ASan+UBSan. Minimal Release passes 819/819 with untracked generated
mode-restoring native launchers (unchanged binaries/arguments/assertions) for the
runner's permission-reset issue. Expanded private constants pass strict C89.
Stack ASan and LeakSanitizer disabled; all stages and foreign runtime acceptance open.

v0.407: validate counted canonical lowercase-hex source/call identities and
nonzero function source contexts before peer creation or original-function entry.
RU: NUL/не-hex ID не усекутся как C-строки и не вызывают исходную функцию.
Raw TCP regression reproduces malformed-source acceptance before the fix, then
verifies rejection without peer/lock mutation, separate process/context mutex
ownership, bad function source/call/context fields, exact reply correlation and
valid recovery with exactly two original calls. Sixteen focused checks pass in
AGG Release and restricted heap/global ASan+UBSan. Minimal Release passes 820/820.
Untracked generated native launchers restore runner-reset executable mode in
minimal/sanitizer without changing binaries, arguments or assertions. Private
constants pass strict C89. Stack ASan and LeakSanitizer disabled. Identity fields
are metadata, not authentication; all stages and foreign runtime acceptance open.

v0.408: use the production ProjectVersion.cmake module to register VERSION.txt
as a configuration dependency, refreshing project metadata on an ordinary build
without manual configure. RU: версия проекта обновляется при изменении только
VERSION.txt. The isolated compiler-free fixture reproduces stale metadata without
the dependency, then passes configure/build/edit/build/rebuild with Unix Makefiles,
Ninja and Ninja Multi-Config on Linux. Registered fixture passes in minimal,
AGG, sanitizer-configured and no-OS build directories; this is a host metadata
check, not new sanitizer runtime evidence. Actual caches report project 0.408.
Full minimal Release passes 821/821, using generated untracked mode-restoring
native launchers for runner-reset permissions; binaries/assertions are unchanged.
The fixture does not modify checkout VERSION.txt and inherits native generator/
make/platform/toolset settings. Windows/macOS/Android runtime and physical LAN
acceptance remain open, as do all nine roadmap stages.

### v0.409 — UTF-16 count memory / Память подсчёта UTF-16

Implemented allocation-free count queries and boxed String length validation;
see [UTF16_STRINGS.md](UTF16_STRINGS.md). Проверка без выделений добавлена;
этапы 1–9 остаются открытыми. Linux minimal Release: 822/822 CTests passed. Generated native-test launchers
restore executable mode before exec because the workspace resets file modes;
test bodies and assertions are unchanged. Other-platform acceptance remains open.

### v0.410 — UTF-16 buffer preflight / Проверка ёмкости UTF-16

Valid short buffers now publish required length without allocation or writes;
malformed input and overlapping copy behavior are preserved.
[Details / Подробности](UTF16_STRINGS.md). All nine stages remain open.

Validation v0.410 / Проверка: Linux minimal Release 822/822; AGG Release
4/4 focused string tests; heap/global ASan+UBSan 4/4. Generated native launchers
restore workspace executable permissions; stack ASan and LSan remain disabled.
No other-platform runtime or complete stage acceptance is claimed.

### v0.411 — one UTF-16 snapshot allocation / Одно выделение снимка

The sufficient-capacity C API copy reserves the measured UTF-16 size exactly,
removing geometric reallocation and transient old/new decoded arrays. Snapshot
semantics for overlapping input/output remain. A 257-unit regression allowing
exactly one allocation fails on v0.410 and passes after this change.
Копирование резервирует точный размер снимка одним выделением памяти; перекрытие
буферов поддерживается. Все девять стадий остаются открытыми.

Validation v0.411 / Проверка: minimal Release 822/822; AGG Release 4/4;
heap/global ASan+UBSan 4/4 string tests. Strict C89 public-header syntax passed.
Native test launchers restore executable modes; stack ASan/LSan are disabled.
Other platforms and complete Flash compatibility remain unverified.

### v0.412 — loaded parseInt / Загруженный parseInt

Separate SWF4–8 parsing now uses radix-before-text coercion, explicit-base
validation, raw hex/octal and counted-prefix rules; unloaded host behavior stays.
[EN](GLOBAL_PARSING.md) / [RU](GLOBAL_PARSING.ru.md). All stages remain open.
User reconfirmed SWF ≤8 only on 16 October; AVM2/AS3/SWF9+ are outside this scope.

Validation v0.412 / Проверка: Linux minimal Release 823/823 CTests; AGG Release
and heap/global ASan+UBSan each passed all three global-parsing executables.
Native launchers restore workspace executable modes; stack ASan/LSan disabled.
Other-platform runtime and all nine stage acceptance remain open.

### v0.413 — loaded parseFloat / Загруженный parseFloat

Separate SWF4–8 decimal accumulation now preserves pinned decimal rounding,
multiple-dot, missing-argument and wrapped-exponent behavior with counted scans.
[EN](GLOBAL_PARSING.md) / [RU](GLOBAL_PARSING.ru.md). All stages remain open.

Validation v0.413 / Проверка: minimal Release 824/824; AGG Release and
heap/global ASan+UBSan each passed all four global-parsing executables.
Native mode-restoring launchers used; stack ASan/LSan disabled.
Other-platform runtime acceptance and all nine stages remain open.

### v0.414 — parsing work/GC/JIT / Бюджет, GC и JIT разбора

Loaded numeric scans share nested AVM1 work and saturated counters. Exact quota,
allocation-failure cleanup and real interpreted/attached/automatic native call
regressions added. [EN](GLOBAL_PARSING.md) / [RU](GLOBAL_PARSING.ru.md).
All nine stages remain open; no complete decoder/coercion budget is claimed.

Validation v0.414 / Проверка: minimal Release 827/827; AGG Release 7/7 and
heap/global ASan+UBSan 7/7 parsing/JIT tests. Native mode-restoring launchers
used; stack ASan/LSan disabled. All nine stage acceptances remain open.

### v0.415 — numeric leading whitespace / Ведущие пробелы чисел

Loaded Number/ToNumber and parseFloat now share Flash's four-unit leading-space
policy; host rules stay. SWF4 invalid-zero vs SWF5–8 NaN, counted surrogate/NUL,
traced GC and actual native entry regressions added.
[EN](GLOBAL_PARSING.md) / [RU](GLOBAL_PARSING.ru.md). All nine stages remain open.

Validation v0.415 / Проверка: minimal Release 829/829; AGG Release 9/9 and
heap/global ASan+UBSan 9/9 parsing/whitespace/JIT tests. Native mode-restoring
launchers used; stack ASan/LSan disabled. Other-platform acceptance remains open.

### v0.416 — shared decimal coercion / Общее десятичное преобразование

Loaded Number/ToNumber/numeric operands share reference-ordered decimal
accumulation with parseFloat; strict SWF5–8, relaxed SWF4 and raw integer radix
policies retained. Primitive decimal conversion removes temporary allocations.
[EN](GLOBAL_PARSING.md) / [RU](GLOBAL_PARSING.ru.md). All nine stages remain open.

Validation v0.416 / Проверка: minimal Release 832/832; AGG Release 12/12 and
heap/global ASan+UBSan 12/12 parsing/decimal/whitespace/JIT tests. Native launchers
restore workspace executable modes; stack ASan/LSan disabled. No other-platform
runtime acceptance or complete stage closure is claimed.

### v0.417 — apply preparation roots / Корни подготовки apply

Boxed receiver and partially captured arguments survive real script-getter GC.
Counted strings, cycles, errors/exceptions and retry covered in host/SWF4–8.
[EN](FUNCTION_APPLY.md) / [RU](FUNCTION_APPLY.ru.md). All stages remain open.

Validation v0.417 / Проверка: minimal Release 833/833; AGG Release 2/2 and
heap/global ASan+UBSan 2/2 apply/function tests. Generated native launchers restore
workspace executable modes; stack ASan/LSan disabled. Foreign runtime acceptance
and all nine stages remain open.

### v0.418 — loaded apply length / Длина загруженного apply

Primitive lists supply zero arguments; own raw length uses version casing and
AVM1 Int32 conversion, with rooted collecting callbacks. Loaded negative/large
lengths remain explicitly bounded. [EN](FUNCTION_APPLY.md) / [RU](FUNCTION_APPLY.ru.md).
All nine stages remain open.

Validation v0.418 / Проверка: minimal Release 834/834; AGG Release 3/3 and
heap/global ASan+UBSan 3/3 apply/function tests. Native launchers restore workspace
executable modes; stack ASan/LSan disabled. Other-platform acceptance remains open.

### v0.419 — apply property capture / Чтение свойств apply

Loaded native Number/Boolean/String/Date lists use ordinary named properties,
original getter receiver and retained snapshots across mutation/GC/failure.
[EN](FUNCTION_APPLY.md) / [RU](FUNCTION_APPLY.ru.md). All stages remain open.

Validation v0.419 / Проверка: minimal Release 835/835; AGG Release 4/4 and
heap/global ASan+UBSan 4/4 apply/function tests. Native mode-restoring launchers
used; stack ASan/LSan disabled. Other-platform runtime acceptance remains open.

### v0.420 — incremental apply work / Постепенный apply с бюджетом

Loaded positive Int32 lists grow rooted capture under shared work; the former
65536 cap is removed. C/script 65537 slots, huge bounded allocation, OOM/unwind
and real native CallMethod checks added. Host/negative-length policy retained.
[EN](FUNCTION_APPLY.md) / [RU](FUNCTION_APPLY.ru.md). All stages remain open.

Validation v0.420: Linux minimal 838/838; AGG 7/7; heap/global ASan+UBSan 7/7.
Generated native-mode launchers compensate for workspace executable-mode resets.
Stack ASan and LeakSanitizer are disabled in this environment. Other platforms
and completion of all nine stages remain unverified/open.

## v0.421 — borrowed adapters and non-callable targets

Loaded SWF4–8 Function.call/apply return undefined when their borrowed receiver
has no invocation callback. Receiver boxing and apply length/index preparation
still happen first: getter errors, counted exceptions and quota exhaustion are
preserved. Host API behavior remains UNSUPPORTED for the same non-callable target;
public direct invocation and callable C/script/remote interfaces retain dispatch.
The behavior follows the pinned Ruffle function.rs reference linked above.

`function_borrowed_adapters` reproduces the old failure and checks all loaded
versions plus host behavior, actual GC in a script getter, counted exception
forwarding, retry, callable dispatch and quota-before-result ordering.
`jit_function_apply` also executes both borrowed adapters through interpreted,
attached and automatic native CallMethod paths. Complete super-adapter semantics,
full Function compatibility and all nine roadmap stages remain open.

Validation v0.421: Linux minimal 839/839; AGG 9/9; heap/global ASan+UBSan
9/9, including remote-function regression. Generated executable-mode launchers
were used; stack ASan/LeakSanitizer remain disabled. No other OS certification.

## v0.422 — super facades require direct dispatch

Loaded Function.call/apply treat a retained super facade as a non-function even
though FlashVM exposes a native callback for direct superclass invocation.
Argument preparation still precedes undefined: script index getters run, may
collect, and propagate errors/counted exceptions. Host borrowed invocation and
public direct invocation of the facade keep their previous parent dispatch.
This follows the pinned Ruffle function.rs explicit as_function/direct-super
separation; it does not certify every super or Function behavior.

`function_super_adapters` first failed against v0.421, then checks loaded SWF5–8
and host mode, both adapters, parent side effects, real getter GC, errors,
embedded-NUL exceptions, retry, direct invocation and root/work cleanup.
Negative signed apply lengths remain explicitly UNSUPPORTED: the reference casts
them to an allocation size, so treating them as zero is not supported by that
source. All roadmap stages remain open.

Validation v0.422: Linux minimal 840/840; AGG 13/13; heap/global ASan+UBSan
13/13, including direct super/owner/prototype and remote-function regressions.
Generated executable-mode launchers were used. Stack ASan/LeakSanitizer remain
disabled; no additional operating system or complete-stage acceptance is claimed.

## v0.423

v0.423 adds a shared allocation-free stored-name selector using the existing
pinned Flash UTF-16 lowercase comparator. Loaded SWF4–6 own membership, attribute
queries and ordinary prototype-chain reads accept equivalent spelling. Host and
SWF7–8 keep exact names. Data, accessor and native method tables are covered;
queries never execute getters. Wrapped native methods use their stored spelling,
so reading a case variant preserves Function identity. Existing own assignments
select stored spelling; default intrinsic __proto__ aliases select its canonical
name. This avoids introducing a duplicate during a legacy update.

`object_property_case_queries` failed on v0.422 and covers SWF4–8/host, ASCII,
U+00C4/U+00E4, embedded NUL and lone-surrogate names, inherited reads vs own
membership, hidden flags, no getter/method execution, wrapped method identity,
legacy updates and real GC. Existing prototype/query tests remain regressions.
Reference: Ruffle commit 2372a4e148a67d68efd796f4477b3cddd4f880cd,
[script_object.rs](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/object/script_object.rs)
uses activation.is_case_sensitive() for property membership and access.

This is a tested subset, not full name compatibility. Case-equivalent duplicate
slots supplied through raw/internal APIs prefer exact spelling and then map order;
Flash collision order is not certified. Flag mutation/deletion, inherited setter
spelling, watchers, definition APIs, extension metatables and full super membership
still need coordinated work. Linear folded lookup is not fully work-budgeted.
The original binary SWF corpus could not be restored through the text-only reader;
no new original-SWF pass count is claimed. All nine stages remain open.

Loaded SWF5+ no longer stores a synthetic ordinary this slot: activation-cell
lookup preserves the SWF5 local special-name rule (pinned function.rs load_this
sets registers only). Existing this/interpreter/JIT tests remain unchanged. The
coordinate test now expects folded public reads in SWF4–6; its coordinate
conversion assertions are retained. The class GC fixture uses ParentClass instead
of Super, which now correctly collides with the local implicit super name.

Validation v0.423: Linux minimal 841/841; AGG 8/8; heap/global ASan+UBSan
8/8. Generated native-mode launchers compensate for workspace mode resets.
Stack ASan/LeakSanitizer remain disabled. Other platforms and all stages open.

## v0.424 — mutate the selected own spelling

Flag updates and deletion select the own stored spelling before mutation. A
legacy alternate name now modifies/removes the same data, accessor or method
as its read and membership query. DONT_DELETE is checked before erasure; successful
deletion clears matching attributes, and inherited properties remain untouched.
Selection snapshots the name before erasing tables and remains inside the C API
allocation-failure boundary. Assignment shares the selector introduced here.
Host/SWF7–8 exact spelling and the pre-existing flags-only API policy remain.

`object_property_case_mutation` reproduced v0.423's mismatched flag update. It
checks SWF4–8/host, data/accessors/methods, NUL/Unicode names, DONT_DELETE failure,
clear/retry/delete, stale flags, inherited ownership, no callback execution and GC.
Inherited setter/watch/definition/collision/budget and full super semantics are
still open; this does not complete a roadmap stage.

Validation v0.424: all 842 Linux minimal cases passed (840 initial successes;
two process tests initially hit workspace child-executable PermissionError, then
passed 2/2 after restoring mode). AGG 6/6; heap/global ASan+UBSan 6/6. Generated
mode launchers used; stack ASan/LeakSanitizer disabled. Other OS/stages open.

## v0.425 — inherited setter selection and native execution

The ordinary setter chain now finds data/accessors with the same versioned name
selector as reads. Own data still shadows inherited setters; host/SWF7–8 exact
matching remains. Setter activation snapshots the accessor's stored name, not the
requested variant. Script setter re-resolution and root ownership therefore refer
to the actual descriptor even when callbacks delete it and detach the prototype.

`object_property_case_setter` first failed on v0.424. It checks counted names with
NUL across SWF4–8/host, receiver/owner/name identity, real GC, owner detachment and
descriptor deletion, plain errors, counted exceptions and invocation/root cleanup.
`jit_property_case_setter` executes the same cases via SetMember in interpreted,
attached and automatic JIT modes and asserts native entries. The allocator sweep
`object_property_case_allocation` covers flags/delete/own data updates, failures
through success, unchanged values/flags on OOM and restored root/setter metadata.
Watchers, definition APIs, duplicate-order semantics, complete scope/name rules
and folded traversal work accounting remain open. All nine stages remain open.

Validation v0.425: Linux minimal 845/845; AGG, NO_OS_PORTS and heap/global
ASan+UBSan 7/7 each. Native entry assertions run on available Linux x64; the
NO_OS_PORTS configuration is a Linux-hosted build, not another OS run. Generated
mode launchers used; stack ASan/LeakSanitizer disabled. All stages remain open.

## v0.426 — own membership through super

Loaded super facade own-membership now delegates to its retained receiver,
matching pinned script_object.rs has_own_property. The query does not inspect
private facade metadata or enumerate/read parent properties. It preserves the
receiver's SWF casing. Host facade behavior remains unchanged. Nested/malformed
receiver-facade chains have a bounded iterative traversal instead of recursive
C++ calls; malformed cycles return FVM_ERROR.

`object_super_membership` failed against v0.425 and covers loaded SWF5–8 plus
host, direct C and borrowed Object.hasOwnProperty.call, counted NUL names,
exact/folded spelling, inherited/missing properties and a throwing AVM1 getter
which membership never invokes. Real GC, malformed-cycle rejection/recovery and
queue transfer to another loaded context are checked. The destination repeats
the queries after collecting. Full super enumeration/flags/deletion/definitions,
independent Flash validation, other transports/platforms and all stages remain
open. This is a query-only compatibility change.

Validation v0.426: Linux minimal 846/846; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 5/5 each. Generated native-mode launchers used; stack
ASan/LeakSanitizer disabled. No independent Flash or other OS certification.

## v0.427 — watcher name selection

Loaded SWF4–6 watch replacement, dispatch, raw old-value lookup and unwatch use
Flash name equivalence. The callback receives the requested counted spelling,
not the stored property spelling. Own-property selection now happens after the
watcher returns, so callback mutation is resolved against the current table.
Host/SWF7–8 keep exact watcher names. Getter-free old-value lookup still preserves
virtual stored data, numeric slots and lazy native methods.

`object_property_case_watch` failed on v0.426's callback-name assertion. It checks
SWF4–8/host, NUL names, old/new/userData arguments, single-record replacement,
case-varied unwatch, self-removal followed by real GC, errors/counting exceptions
and running/root cleanup. Primary reference: pinned script_object.rs watch,
unwatch and call_watcher, which passes the requested name to Watcher.call.
Exception-side writeback, watcher recursion identity/collision details, complete
movie-clip watch behavior and budgeted folded traversal remain open. The existing
exception policy is retained here; no complete stage is claimed.

Validation v0.427: Linux minimal 847/847; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 5/5 each. Generated mode launchers used; stack ASan and
LeakSanitizer disabled. Other OS execution and all-stage acceptance remain open.

## v0.428 — case-aware virtual replacement

Loaded SWF4–6 addProperty selects an existing own stored spelling before checking
attributes and replacing its descriptor. Stored data and flags survive replacement;
a variant does not create a second virtual property beside the original data.
Host/SWF7–8 preserve distinct names. The existing null-setter and rollback rules
are retained. This follows pinned script_object.rs add_property_with_case.

`object_property_case_definition` first failed on v0.427. It checks SWF4–8/host,
NUL names, retained flags/data, a collecting script getter, no premature getter
execution and writable retained data with a null setter. An allocation sweep runs
through failure to successful definition, checking previous values/flags and roots.
`jit_property_case_watch` additionally checks requested watcher names, collecting
self-removal and errors via interpreted/attached/automatic native SetMember, with
actual native-entry assertions. Native C definitions, collisions, watcher throw
writeback, complete property semantics and all stages remain open.

Validation v0.428: Linux minimal 850/850; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 8/8 each. Generated mode launchers used; stack ASan and
LeakSanitizer disabled. No other OS or complete-stage certification.

## v0.429 — rooted C definitions and stored names

Rooted native method/accessor definition selects the existing own spelling in
loaded SWF4–6 before applying the existing conflict policy. Replacing a method or
accessor no longer introduces a hidden case-equivalent duplicate; a data/accessor
conflict which fails for exact spelling also fails for its equivalent name.
Host/SWF7–8 keep distinct names. C89 signatures and ABI are unchanged.

`object_property_case_native` reproduced v0.428's duplicate-method failure. It
checks SWF4–8/host, counted NUL names, replacement callbacks/capture roots, method
and accessor table identity, rejected data conflicts, actual GC and invocation of
a previously pinned Function which retains its old callback/capture. Native API
policy is an embedding contract aligned with loaded lookup; it is not an Adobe C
API claim. Stale unexported method caches, full callback lifetime/collision rules,
watcher exception semantics and all stages remain open.

Validation v0.429: Linux minimal 851/851; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 4/4 each. Generated mode launchers used; stack ASan and
LeakSanitizer disabled. Other OS and complete-stage acceptance remain open.

## v0.430 — method cache capture release

v0.430 invalidates stale lazy Function wrappers after a successful native method
replacement, method deletion, ordinary data overwrite or native accessor
replacement. Method-table entries shadowed by published data/accessors are also
removed. Cleanup happens after fallible publication, preserving the old entry and
wrapper on OOM. Identical callback/userdata/root definitions keep Function identity.

[GC:] A retained or pinned Function owns its own callback capture. Removing the
owner's cache does not change that exported Function or its callable behavior.
An unexported wrapper no longer keeps its obsolete capture alive solely through
a pinned owner object. This applies to host and loaded contexts; existing naming,
readonly and deletion checks still run first. No public ABI change.

`gc_method_cache` first failed on v0.429. Across host/SWF4–8 it tests all four
operations with and without a pinned old Function, identical-definition identity,
actual collection, safe live-registry checks before dereferencing, old callback
invocation while pinned and release after unpin. `gc_method_cache_allocation`
sweeps every failure through success in host/SWF6/SWF8, checking old value,
Function identity and capture retention on OOM, then release on success.

This is not complete callback or GC certification. Global registration caches,
other wrapper/resource subsystems, finalizer resurrection and concurrent RPC
lifetimes require their own acceptance. All nine roadmap stages remain open.

Validation v0.430: Linux minimal 853/853; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 6/6 each. Generated mode launchers used; stack ASan and
LeakSanitizer disabled. Other OS and complete-stage acceptance remain open.

## v0.431 — raw Array length in case-varied watchers

Getter-free watcher old-value lookup recognizes Array resource length with the
same counted Flash case rule as ordinary loaded names. SWF4–6 writing LENGTH
while watching length receives old length 3, requested name LENGTH and new value
5; returning 4 resizes the Array to four elements. SWF7–8 preserve distinct names;
host native-length watch policy is unchanged. NUL suffixes remain distinct.

`array_case_watch_length` failed on v0.430's old-value assertion, then checks
host/SWF4–8, callback arguments, actual getter-free resource value, real GC,
resize and NUL-suffix isolation. `jit_array_case_watch_length` repeats resize via
interpreted, attached and automatic native SetMember with native-entry assertions.
This supplements the pinned watcher/property-name reference; complete Array
accessor overrides, exception writeback, budgets and all stages remain open.

Validation v0.431: Linux minimal 855/855; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 5/5 each. Generated mode launchers used; stack ASan and
LeakSanitizer disabled. Other OS execution and complete-stage acceptance open.

## v0.432 — thrown watcher assignment

v0.432 completes loaded Object/Array assignment after a script watcher throws:
the assigned value becomes undefined. A successful own write then returns the
original exception; readonly data stays unchanged and still returns the exception.
Own virtual setters run with undefined and the original watcher exception wins
over a later setter exception. Inherited virtual dispatch returns the setter's
result, consuming the watcher exception when the setter succeeds; its own throw
is propagated. Host watcher failure policy is unchanged.

The source basis is Ruffle commit 2372a4e148a67d68efd796f4477b3cddd4f880cd:
[object.rs set_internal](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/object.rs)
and [script_object.rs call_watcher](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/object/script_object.rs).
The latter changes a thrown watch value to undefined; set_internal continues
assignment and treats inherited setters separately from own set_local.

[GC:] FVM_WatchWriteScope saves the thrown graph before clearing pending state.
Its explicit root frame protects counted strings, objects and cycles through
setter execution and collection. Destruction restores pending ownership before
unlinking the saved frame. It unwinds before the enclosing lookup-owner root.
Saving may allocate; failure before suspension leaves the original pending
exception intact. Ordinary non-throwing writes allocate no saved-value vector.

`object_watch_throw` first failed on v0.431. Host/SWF4–8 cover named data,
Object/Array numeric slots, readonly data, own/inherited script accessors, setter
success/throw and error priority. A self-cyclic object thrown by the watcher
survives a real collecting setter; pending state is suspended inside that setter.
Existing counted-string/interpreter/native watch fixtures now expect undefined
writeback for loaded throws. Raw runtime FVM_ERROR and allocation failures retain
fail-fast handling; their equivalence to reference errors is not certified.
Custom native length/resource setters, metatables, recursion limits and complete
watcher/SWF compatibility require further checks. All nine stages remain open.

Validation v0.432: Linux minimal 856/856; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 6/6 each. Generated native-mode launchers used; stack ASan
and LeakSanitizer disabled. Other operating systems and all stages remain open.

## v0.433 — Array growth excludes inherited virtual dispatch

An indexed loaded Array write now resolves whether a virtual property is inherited
before growing length. Inherited setters, including getter-only virtual properties,
do not enter Array.set_local, so they do not grow the receiver. Own writes retain
growth before readonly checks and own setter invocation. This refines the earlier
blanket growth-before-setter policy. Pinned object.rs set_internal dispatches
inherited setters before script_object.rs set_local performs native Array growth.

`array_inherited_setter_length` failed on v0.432: an inherited index 5 setter saw
length 6 instead of 0. Host/SWF4–8 checks own/inherited dispatch, real GC, setter
throws, thrown watchers and priority/consumption rules; inherited writes leave no
own slot. The native fixture repeats all loaded cases in interpreter/attached/
automatic JIT modes, asserting native entry counts and length inside the setter.
Full Array side effects, resource-specific errors, malformed graphs and work
accounting remain open. No complete stage or platform certification is implied.

Validation v0.433: Linux minimal 858/858; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 5/5 each. Generated native-mode launchers used; stack ASan
and LeakSanitizer disabled. Other OS and all-stage acceptance remain open.

## v0.434 — getter-only dispatch and deferred-error acceptance

A loaded named write through an inherited native getter-only virtual property
now consumes the watcher exception, as inherited call_setter with no setter does.
It leaves no own value and does not execute the getter. Own getter-only properties
still return the watcher exception. Numeric dispatch already followed this rule.
`object_watch_getter_only` reproduced v0.433's named-path mismatch and checks
SWF4–8, own/inherited/numeric/named variants, counted exceptions and real watch GC.

`jit_object_watch_throw` executes the complete data/readonly/own/inherited setter
matrix with cyclic thrown objects and collecting setters in interpreter, attached
and automatic JIT, asserting actual native entry counts. Allocation failures are
swept until the first completed throw-writeback in `object_watch_throw_allocation`:
host/SWF4–8 retain old data on OOM, preserve an already pending exception when
suspension or subsequent preparation fails, clear state and retry. Saved exception
payload and root/watch/setter cleanup are checked after every boundary. These
checks cover own raw writes, not every allocator failure inside a setter.
Other error mappings, resource setters, recursion/collision limits and all nine
roadmap stages remain open. Native-accessor policy remains an embedding contract.

Validation v0.434: Linux minimal 861/861; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 6/6 each. Generated native-mode launchers used; stack ASan
and LeakSanitizer disabled. Other OS execution and all-stage acceptance open.

## v0.435 — borrowed definitions on super

Loaded borrowed Object.addProperty and Object.watch acknowledge valid arguments
on a super facade without storing descriptors/watchers on it. Object.unwatch
returns false for that facade. Name coercion and argument validation still run,
including errors from a coercion callback. Loaded watch rejects a super facade
as its callback: super has direct-call machinery, not ordinary Function identity.
Host behavior is retained. Direct super.method calls still bind the underlying
receiver; this change concerns explicitly borrowed methods with this = super.

`object_super_definitions` reproduced v0.434's stored accessor. It checks SWF5–8
and host, counted NUL names, accepted/rejected arguments, exact coercion counts,
coercion exceptions, descriptor/watcher absence, callback rejection and GC.
Primary references: pinned globals/object.rs add_property/watch/unwatch and
script_object.rs add_property_with_case/watch/unwatch, which decline mutation
of NativeObject::Super. Full facade reflection, native definition APIs, accessor
callback projection and all roadmap stages remain open.

Validation v0.435: Linux minimal 862/862; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 5/5 each. Generated native-mode launchers used; stack ASan
and LeakSanitizer disabled. Other OS execution and all-stage acceptance open.

## v0.436 — super is not a virtual Function callback

Loaded script-property getter/setter dispatch now excludes super facades from
ordinary Function callbacks. A non-function getter uses retained property data;
a non-function setter has no executable side effect and writable data still
updates. The facade remains valid as an Object argument to addProperty. Host
behavior and explicit direct superclass invocation remain unchanged. Pinned
script_object.rs call_getter/call_setter require as_function before execution;
Property retains data independently of getter/setter objects.

`object_super_accessors` first failed on v0.435's retained-data read. Across
host/SWF5–8, a constructor side effect detects accidental parent execution during
get/set, real GC checks descriptor-held facade graphs, direct super still executes
the parent, and queue transfer to another context repeats the retained-data and
side-effect checks after GC. `jit_object_super_accessors` executes GetMember and
SetMember in interpreter, attached and automatic JIT, asserting native entries.
This covers ordinary descriptor dispatch, not complete super/reflection or RPC
semantics. Callback/resource equivalence and all nine stages remain open.

Validation v0.436: Linux minimal 864/864; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 6/6 each. Generated native-mode launchers used; stack ASan
and LeakSanitizer disabled. Other OS and all-stage acceptance remain open.

## v0.437 — setter replacement spelling

After an own script setter returns, loaded SWF4–6 reselect the current own
property spelling before checking readonly flags and committing stored data.
A callback may delete `Slot` and recreate `sLOT` as ordinary data or a virtual
property; the original input updates that replacement without invoking its
setter again. A readonly replacement keeps its data. Host and SWF7–8 keep
exact spelling. This follows pinned Ruffle script_object.rs set_local's
case-aware property entry lookup after executing the setter.

object_setter_case_replacement reproduced the old duplicate-slot write, then
checks all three replacements, host/SWF4–8, a counted NUL string, collection
inside the setter and traced-string collection after the write.
jit_object_setter_case_replacement repeats loaded cases through interpreted,
explicit and automatic JIT SetMember with actual native entries. Native
accessor replacement, broader collisions and all nine stages remain open.

Validation v0.437: Linux minimal 866/866; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 4/4 each. Executable-mode launchers applied; stack
ASan/LeakSanitizer disabled. All nine stages and foreign OS execution remain open.

## v0.438 — borrowed Array raw length casing

Loaded borrowed Array methods now find own raw `LENGTH` data/virtual storage
as `length` in SWF4–6. Raw writes select the existing own spelling before
readonly checks and descriptor-data updates. No getter/setter is executed.
SWF7–8 remain case-sensitive. This follows pinned script_object.rs
length/get_data/set_length/set_data and globals/array.rs push/splice.

array_borrowed_length_case reproduces the former empty slice with `LENGTH=2`,
and checks slice/push/splice for ordinary, virtual and readonly virtual data,
SWF4–8, retained spelling, NUL-suffixed separate names and callback counts.
jit_array_borrowed_length_case repeats the matrix with real native CallMethod
entries. Broader Array behavior, all resources and all nine stages remain open.

Validation v0.438: Linux minimal 868/868; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 4/4 each. Executable-mode launchers applied; stack
ASan/LeakSanitizer disabled. Foreign OS execution and all stages remain open.

## v0.439 — borrowed Array methods on super

The loaded raw own-data write bridge ignores super facades. In particular,
Array.push/unshift/splice borrowed with Function.call no longer create private
indices or a length slot on escaped super. Return values and direct superclass
calls retain their contracts; host writes are unchanged. Pinned
script_object.rs set_element/set_length explicitly return without storing on
NativeObject::Super. This correction concerns Array raw writes, not a claim
about every ordinary super assignment or C extension operation.

array_borrowed_super reproduced the facade mutation, then runs push, unshift,
splice, slice, pop, shift and reverse successively for SWF5–8. It checks return
values, unchanged raw storage and receiver state, actual GC after each method,
and clean call/root state. jit_array_borrowed_super repeats all calls in the
interpreter and two native modes, asserting actual entries. Full super/Array,
transport and all nine acceptance stages remain open.

Validation v0.439: Linux minimal 870/870; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 4/4 each. Executable-mode launchers applied; stack
ASan/LeakSanitizer disabled. Foreign OS execution and all stages remain open.

## v0.440 — registerClass rejects super constructors

Loaded Object.registerClass rejects a super facade as a constructor before
coercing the linkage name. A callable parent interface is not a Function
constructor. Invalid registration leaves the existing constructor intact.
Host acceptance and null/undefined unregistration remain unchanged. Pinned
globals/object.rs register_class validates as_function before string coercion.

object_register_class_super reproduces the old acceptance/name callback, then
checks SWF5–8 and host, NUL linkage keys, callback counts, suppression of a
throwing name conversion for invalid constructors, propagation for valid
constructors, existing registration preservation, successful retry and null
unregistration followed by GC. jit_object_register_class_super repeats
registration through interpreted/explicit/automatic CallMethod with real
native entries. General constructor semantics and all stages remain open.

Validation v0.440: Linux minimal 872/872; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 4/4 each. Executable-mode launchers applied; stack
ASan/LeakSanitizer disabled. Foreign OS execution and all stages remain open.

## v0.441 — loaded super TypeOf

The shared TypeOf helper reports `object` for a loaded super facade even though
it supports explicit parent invocation. Real Function objects still report
`function`; ordinary instances report `object`. Host behavior is retained.
The allocation-free classification follows pinned avm1/value.rs type_of,
which recognizes Functions through as_function rather than generic callability.

object_super_typeof reproduces the previous `function` result, then checks
host/SWF5–8, in-place C helper output, actual Functions, instances, collection
and repeat, no constructor side effect, and preservation of direct super
invocation. jit_object_super_typeof repeats TypeOf in interpreted, explicit
and automatic JIT execution with real native entries. Other super reflection,
conversion and all nine stages remain open.

Validation v0.441: Linux minimal 874/874; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 4/4 each. Executable-mode launchers applied; stack
ASan/LeakSanitizer disabled. Foreign OS execution and all stages remain open.

## v0.442 — super string fallback

Loaded ToString now falls back to `[type Object]` for a super facade when its
parent-visible toString returns a non-string. Generic call support does not
make the facade a Function; actual Functions keep `[type Function]`. String
returns (including NUL) and thrown values pass through unchanged. Host is
unchanged. Pinned avm1/value.rs coerce_to_string checks as_function only after
executing the special toString method. Receiver/owner dispatch is a separate
remaining concern; this change only corrects the fallback classification.

object_super_string reproduced the previous Function fallback. It checks
SWF5–8, number/undefined/null/string/exception returns, callback count, in-place
C helper, Function/instance controls, GC and repeatability of direct parent
invocation. jit_object_super_string repeats ToString in three execution modes
with actual native entries. Full conversions and all stages remain open.

Validation v0.442: Linux minimal 876/876; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 4/4 each. Executable-mode launchers applied; stack
ASan/LeakSanitizer disabled. Foreign OS execution and all stages remain open.

## v0.443 — special parent conversion receiver and owner

Loaded numeric/default/string conversion of super now uses parent-method
dispatch with the retained original receiver and the selected defining owner.
Missing or noncallable conversion members yield undefined; real callee errors
propagate. Owner selection uses versioned spelling and is rooted before a
getter can detach it. Existing explicit-call error behavior and host conversion
are retained. Pinned object.rs and super_object.rs call_method distinguish
Special calls and execute against super.this(), with parent lookup depth.

object_super_coercion_receiver reproduced the previous facade receiver. It
checks ToNumber, ToString and default primitive conversion in SWF5–8, folded
SWF5–6 names, primitive/noncallable Object members, number/NUL string results,
exception/plain error, collection inside callbacks and self-deletion plus
prototype detachment. Callback receiver/owner and cleanup are asserted.
jit_object_super_coercion_receiver repeats ToNumber/ToString/Add2 in three
modes with actual native entries. Broad resolution/owner mutation, failure
boundaries and all nine stages remain open.

Validation v0.443: Linux minimal 878/878; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 4/4 each. Executable-mode launchers applied; stack
ASan/LeakSanitizer disabled. Foreign OS execution and all stages remain open.

## v0.444 — allocation-free invocation identity roots

FVM_CallScope now retains saved/active constructor, function and owner through
six inline root frames. Publishing the complete chain and restoring the
previous identity no longer allocate a temporary vector. Nested calls and
unwinding preserve the caller's root head. This is an implementation/GC
change, not an expansion of SWF compatibility.

super_conversion_allocation reproduced failure to enter a call scope with all
new/new[] denied. It now enters/exits and unwinds a nested scope under that
restriction, collects with all six identities live, and verifies reclamation
after detachment. It also sweeps every new/new[] boundary up to first completed
ToNumber/ToString/Add2 success or thrown value across SWF5–8, using fresh
contexts. Failures preserve aliased output, reset recursion/invocation roots,
retain any pending thrown payload and permit GC/retry. These are C++
allocation boundaries, not every OS/C allocator or full concurrent GC proof.
All nine stages remain open.

Validation v0.444: Linux minimal 879/879; minimal/AGG/Linux-hosted NO_OS_PORTS/
heap-global ASan+UBSan targeted checks 5/5 each after dependency-cache repair.
The first sanitizer run exposed a stale function.cpp object using the old
CallScope layout; regenerating dependency caches rebuilt it and the check
passed. Executable-mode launchers applied; stack ASan/LeakSanitizer disabled.
Foreign OS execution and all stages remain open.

## v0.445 — legacy super prototype aliases

Loaded SWF5–6 case aliases of super.__proto__ now project the selected ancestor
prototype, as the canonical spelling already did. The override scan uses the
same versioned name selection, so a differently cased own getter is respected
before the intrinsic projection. SWF7–8 remain exact; NUL suffixes are distinct.
This combines pinned super_object.rs proto/base_proto with the case-aware
script_object.rs property map. It does not complete ordinary raw __proto__
storage or prototype mutation semantics.

object_super_prototype_case reproduced the old receiver-prototype result for
uppercase aliases. It checks canonical/upper/mixed/NUL names in SWF5–8, queue
transfer with GC and copied receiver topology, canonical/upper user getters,
actual receiver and callback counts. jit_object_super_prototype_case repeats
GetMember in interpreted/explicit/automatic modes with actual native entries.
Full prototype/reflection behavior and all stages remain open.

Validation v0.445: Linux minimal 881/881; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 4/4 each. Executable-mode launchers applied; stack
ASan/LeakSanitizer disabled. Foreign OS execution and all stages remain open.

## v0.446 — sortOn raw field casing

Loaded Array.sortOn now applies SWF4–6 case equivalence to own stored fields,
virtual descriptor data and native method entries. Array LENGTH selects native
length storage under the same version rule. Method wrappers use the actual
stored name for cache identity. SWF7–8 remain exact, inherited fields remain
excluded and getters/setters are not called. Host sorting is unchanged. Pinned
globals/array.rs sort_on_compare uses get_local_stored; script_object.rs
get_local_stored/property map apply activation case sensitivity.

array_sort_on_case reproduces the previous unsorted differently cased field.
It checks SWF4–8, ordinary/virtual/inherited/Array-length/native-method fields,
counted NUL and accented Unicode names, numeric/text comparison, normal/unique/
indexed return policies, unchanged indexed-return input, zero callbacks, stored
method cache identity and GC cleanup. jit_array_sort_on_case repeats the matrix
with actual interpreted/explicit/automatic CallMethod entries. Full sorting,
collision/budget rules and all stages remain open.

Validation v0.446: complete Linux minimal 883/883 and AGG 902/902;
AGG/Linux-hosted NO_OS_PORTS/heap-global ASan+UBSan targeted checks 4/4 each.
The full AGG profile includes fonts/image codecs/SQLite and render jobs;
it is not a separate Cairo profile or foreign-OS run. Executable-mode
launchers applied; stack ASan/LeakSanitizer disabled. All stages remain open.

## v0.447 — C method call case and getter owner roots

The counted C method-call bridge now uses the shared SWF4–6 name selector for
data/accessor dispatch and native-method boundaries. Own Function data shadows
an inherited native method regardless of legacy-equivalent spelling. Host and
SWF7–8 remain exact. The selected receiver/owner stay rooted while a getter
detaches the property/prototype and the returned method collects. The loop uses
the named prototype-depth bound. C unsupported-name behavior is retained.

object_call_case reproduced the previous failed folded call, then checks
host/SWF4–8, exact/folded counted NUL names, own/inherited data Functions,
virtual getters, native methods and native shadowing, exceptions and callback
counts. Collecting script methods verify original receiver/owner; a getter
can delete itself and detach the chain before that collection. The shared
case rule follows pinned AVM1 object.rs call_method/search_prototype and
script_object.rs property lookup; C failure policy is an embedding contract.
Broader call inputs, resolver behavior and all stages remain open.

Validation v0.447: complete Linux minimal 884/884; AGG, Linux-hosted NO_OS_PORTS
and heap/global ASan+UBSan 4/4 targeted checks each. Executable-mode launchers
applied; stack ASan/LeakSanitizer disabled. All stages/foreign OS runs remain open.

## v0.448 — C call argument snapshot before getter

The C method-call bridge copies argument values into a traced vector and roots
the receiver before resolving a getter or clearing the output. A collecting
getter can mutate the original argument array without changing the prepared
call, and output may alias an argument. Managed string/Object/metatable edges
remain live; external string bytes remain caller-owned. The same snapshot
covers super and trusted native entries, replacing their redundant local
argument copy/root boilerplate. Ordinary native barriers are retained.

object_call_argument_snapshot reproduced collection of the pending cyclic
Object inside a getter. It checks host/SWF4–8, traced counted NUL strings,
cyclic object arguments, caller-array mutation, aliased output, getter/callee
exceptions, callback counts, cleanup and reclamation after return. This is an
embedding lifetime contract; argument cloning/external buffers, native capture
lifetime, failure boundaries and all stages remain open.

Validation v0.448: complete Linux minimal 885/885; AGG, Linux-hosted NO_OS_PORTS
and heap/global ASan+UBSan 4/4 targeted checks each. Executable-mode launchers
applied; stack ASan/LeakSanitizer disabled. All stages/foreign OS runs remain open.

## v0.449 — active native capture root

The C method-call bridge roots the selected native entry's capture for the
whole callback. A trusted collecting callback may delete/replace its method
or global fallback registration without losing the active userdata graph.
The root ends after the call; replacement is not rolled back. Public ordinary
native callbacks retain their collector barrier. collector_entry is internal
trusted-bridge state, not a public extension opt-out.

object_call_native_capture reproduced capture collection after self-removal.
It checks host/SWF4–8, own deletion/inherited replacement/global replacement,
cyclic captures and argument graphs, traced NUL strings, caller-array mutation,
OK/exception/plain error, GC during callbacks, pin/pending-exception retention
and reclamation after call/clear/unpin. Broader entry/finalizer/concurrency
coverage and all stages remain open.

Validation v0.449: complete Linux minimal 886/886; AGG, Linux-hosted NO_OS_PORTS
and heap/global ASan+UBSan 4/4 targeted checks each. Executable-mode launchers
applied; stack ASan/LeakSanitizer disabled. All stages/foreign OS runs remain open.
