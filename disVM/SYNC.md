# v0.1661 — Bio bounds and executable local type scopes

Current pin: **6a307e5d23697d62d7bcb841fbddc1c80d0fd975**, **703 Dis / 222 bootstrap files**. Incoming Bio corrections bound byte/rune pushback before state mutation, widen buffered/position arithmetic, and propagate failed logical-position queries before relative seeking. The source bootstrap imports the actual library bodies and new 35,658-case sanitizer fixture. The Dis core does not link an unrelated Bio service; its selected checkpoint reference advances with the same source pin.

Both snapshots remain byte-for-byte upstream and earlier reverse corrections are retained, including c1494cd's shared-inline query cache. No additional confirmed shared defect required a reverse commit in this series. Our local type scopes are FlashVM compiler work and stay out of Inferno. OS file operations remain behind existing host ports.

[Release](../docs/releases/v0.1661.md) · [Alias contract EN](../fir/v1/doc/SOURCE_ALIASES.md) / [RU](../fir/v1/doc/SOURCE_ALIASES.ru.md).

## Previous checkpoints

# v0.1660 — JIT GC geometry and shared inline dependency graphs

Current pin: **c1494cdc5f20cb5399c4dcdffddd03701bd8644b**; **703 selected Dis / 219 bootstrap files**. Both snapshots retain exact imported bytes and manifest hashes.

Incoming 139ef17 shares nonoverlapping Heap.ref/Heap.color and complete disjoint GC-cell preflight across AMD64/ARM64 owner leaves, and rejects invalid builtin type sentinels for formatting/byte conversion. Incoming 6dcfe7b removes recursive inline-dependency traversal and refuses active candidate cycles while restoring temporary flags.

Our reverse [c1494cd](https://github.com/bhgv/Inferno-OS-bhgv/commit/c1494cdc5f20cb5399c4dcdffddd03701bd8644b) bounds repeated expansion of shared successful inline candidates. A compact 256-candidate diamond previously took exponential work; an old-source replay compiled successfully but exceeded the execution alarm. The fixed checker caches successful bodies only inside one query and restores active/completed flags on acceptance, rejection and successful retry. Ordinary shared AST child edges are not memoized. Allocation/storage exhaustion remains fatal; the pass is single-threaded.

Native/emulated owner fixtures, complete dispatch, emitter layouts, deep/shared inline graphs and source bootstrap checks cover the imported code. The FlashVM-only alias-typed GC tutorial exercises formatting, narrowing, deleted-key snapshots and table mutations through our Coco/R → FIR → Dis path, with real GC scheduling and both dispatch modes. It does not substitute for hardware ARM64 acceptance or a complete ABI migration. Flash-specific table/any/projection extensions stay out of upstream.

[Release EN/RU](../docs/releases/v0.1660.md) · [Tutorial EN](../fir/v1/examples/type-aliases/README.md) / [RU](../fir/v1/examples/type-aliases/README.ru.md).

## Previous checkpoints

# v0.1659 — Source binding integration and verified upstream head

Inferno experiment was fetched again and remains **2bd9f3cdcfbe21d39b6829d6440dc86662d2c951**. Both reverse corrections from the previous series (b151e32d occurs traversal and 2bd9f3c cold ARM64 fixture identity) are present in its history. Both manifests still verify **699 Dis / 216 bootstrap files** byte-for-byte. No additional shared defect was confirmed in this pass, so no duplicate upstream commit was created.

FlashVM-owned executable source aliases now resolve forward references and cycles before FIR generation, and alias casts execute through existing Dis conversion operations. These frontend extensions remain in FlashVM. No upstream opcode, native frame ABI or selected JIT source is changed by this series. Both interpreter/JIT acceptance and relocated compiler/runtime SDK checks exercise the new source path.

[Release](../docs/releases/v0.1659.md) · [EN contract](../fir/v1/doc/SOURCE_ALIASES.md) / [RU](../fir/v1/doc/SOURCE_ALIASES.ru.md).

## Previous checkpoints

# v0.1658 — Sys capture, bounded AST queries and JIT synchronization

Current pin: **2bd9f3cdcfbe21d39b6829d6440dc86662d2c951**, **699 selected Dis files**; the independent source bootstrap selects **216 files** at the same revision. Manifests retain individual SHA-256 hashes and imported files remain byte-for-byte upstream.

Incoming 56e3f67 captures Sys.pctl frames and validates packed FD lists before owner access, including malformed/cyclic input. 3523d0d removes deep varinit/caselist/right-occurs recursion. fac323c bounds JIT layout arithmetic and ARM64 INDC capacity. Later aea8343 bounds further compiler tree queries; f77c625 rejects unaligned ARM64 JIT-to-C targets and expands dispatch fixtures. Incoming 8410b0b bounds additional node/typecheck transforms; 38ed47e validates six runtime callback identities and corrects foreign ARM64 fixture alignment.

Our reverse [b151e32d](https://github.com/bhgv/Inferno-OS-bhgv/commit/b151e32daa309b3f6030363a39717091a430d642) removes remaining left/mixed occurs recursion with an inline worklist and checked heap growth. A 100000-node left tree reproduced ASan stack overflow with the previous code under a 256 KiB stack. Tests preserve per-edge DAG counts, terminal name handling and read-only input. This is not a claim that all compiler traversals accept arbitrary cyclic graphs. The reverse fix is already an ancestor of the final incoming pin.

Our second reverse [2bd9f3c](https://github.com/bhgv/Inferno-OS-bhgv/commit/2bd9f3cdcfbe21d39b6829d6440dc86662d2c951) explicitly aligns the cold cjitexec fixture identity for foreign ARM64 publication. Both WORD widths now pass with unsanitized -O1 and UBSan, without weakening the production target guard.

Selected pctl fixtures run UBSan in bounded frame/node configurations; node-chain/query fixtures exercise extracted real compiler bodies under ASan/UBSan. LSan process inspection is unavailable in this sandbox and explicitly disabled by these fixtures. Emitter/dispatch fixtures do not constitute execution on ARM64 hardware. Flash-specific table/any/projection/compiler extensions stay in FlashVM.

[Release EN/RU](../docs/releases/v0.1658.md) · [Executable aliases EN](../fir/v1/doc/SOURCE_ALIASES.md) / [RU](../fir/v1/doc/SOURCE_ALIASES.ru.md).

## Previous checkpoints

# v0.1657 — Dial/file2chan synchronization and outer alias binding

Current pin: **0ac9d293660a718cb3dbc3472e9c40d6b31cca8e**, **683 selected files**.
Incoming 275bc64 bounds dial paths/counts and fixes relative clone translation;
3ba10d2 captures file2chan frames/results and owners. Our confirmed reverse
[0ac9d293](https://github.com/bhgv/Inferno-OS-bhgv/commit/0ac9d293660a718cb3dbc3472e9c40d6b31cca8e)
rejects incomplete/oversized control writes before data opens or descriptor
ownership transfers. Both full dial port bodies run ASan/UBSan fixtures.

Our Coco/R outer alias binding is FlashVM-owned compiler work. Inferno already
supports aliases; it receives only the shared native dial correction, not our
frontend or Flash-specific extensions. [Release](../docs/releases/v0.1657.md).

## Previous checkpoints

# v0.1656 — Bidirectional Sys capture and pipe-layout synchronization

Current pin: **9ee638bbfaff421164e2e7fb1d68e01089720c39**, **677 selected files**.
Six incoming Sys commits since 9a87d74 are imported with their tests. Our confirmed
pipe element-layout correction is published back to experiment as
[9ee638bb](https://github.com/bhgv/Inferno-OS-bhgv/commit/9ee638bbfaff421164e2e7fb1d68e01089720c39).
It prevents pointer-sized owner access in a non-pointer array, in emu and os.
No Flash any/table/projection extensions were exported. No incoming JIT backend
changes occur in this range; selected existing native/compiler checks pass.

The independent NC64 host separately captures twelve Sys builtin boundaries and
validates packed complete results/old owners. Runtime 216/216, final focused 26/26,
10 upstream Sys drivers and 5 JIT/compiler drivers pass; guard and new pipe
harnesses pass ASan/UBSan (unsupported LSan process inspection disabled).
[EN contract](runtime/SYS_CAPTURE.md) / [RU](runtime/SYS_CAPTURE.ru.md) ·
[Release](../docs/releases/v0.1656.md).

## Previous checkpoints

# v0.1653 — Sys frames/owners and mapped capture refresh

Current pin / Текущий снимок: **9a87d74eb4f47cf7b8ea35c804f86c7c20e40d36**.
Exact selections / Точные выборки: **662 core/reference/library + 207 bootstrap files**.
Incoming changes validate complete Sys UTF/tokenizer/public-format frames, packed
views and owners before side effects, and capture mapped-copy operands across
destructor callbacks. Original C, complete AMD64 JIT and WORD-width tests retained.
Full core 248/248 at 047e0dc plus final affected 9a87d74 refresh 6/6 passed.

Bidirectional review / Двусторонняя сверка: experiment retains our previous
reverse fixes, including 73a6b5a. No new shared defect was found for a reverse patch;
AS2/FIR/IDE changes are flashVM-owned and were not copied into Inferno.
[RU/EN release](../docs/releases/v0.1653.md) ·
[AS2 → FIR → SWO → SWF](../compiler/as2/view/README.md).

## Previous checkpoints / Предыдущие состояния

# Graphical browsing and codec owner guards — v0.1652

Current pin: **3760fc951987915d4c823ba30f56f07386e98b18**.
Exact selections: **650 core/reference/library files + 207 bootstrap files**.
Incoming 3760fc9 encodes packed Rune exports through aligned locals and validates
codec result owners/descriptors before side effects in C and AMD64/ARM64 JIT.
Original codec-owner C/emitted-code regressions are selected and registered.

Bidirectional review confirms this revision contains our preceding reverse
73a6b5a reduced-address fixture fix. No additional shared defect was identified
in this series; the new browser state/controller belongs to FlashVM IDE and is
not copied into Inferno. OS access stays in existing ports.
[EN](../ide/dis/GRAPHICAL-BROWSER.md) / [RU](../ide/dis/GRAPHICAL-BROWSER.ru.md).

## Previous checkpoint

# Directory owners and Sys format guards — v0.1651

Current pin: **73a6b5a972ea8854fa8b593569c97a68da0b0748**.
Exact selections: **646 core/reference/library files + 207 bootstrap files**.
Incoming 55be356 bounds formatter output, arguments and borrowed String metadata,
and preserves native Heap reference-counter width. Its sys-print-spans fixtures
are selected and registered; original upstream files remain byte-for-byte.

The final fence also imports 64b8855 and e9b187d: packed String UTF views and
comparisons use byte-safe reads; mutable string owners and expansion are checked
before side effects. AMD64/ARM64 append/store/slice leaves reject unaligned
owners before touching their metadata. Their new C and emitted-code regressions
are retained, including explicit ownership/cache-failure probes.

Bidirectional review: the final revision includes preceding reverse ff21d3c fixes.
Reverse **73a6b5a** repairs two older reduced-address string test adapters: they
retain 16-bit overflow geometry but use native payload pointers for actual data
access. All original extent/cache/ownership oracles remain. The 7260 cell and
420 UTF-view cases per WORD model pass ASan/UBSan. Production string handlers
are unchanged by this reverse patch.

The explicit quota correction belongs to this hosted runner; directory ownership
and console policy belong to FlashVM IDE, so these adapters are not copied into
Inferno.
[EN directory contract](../ide/dis/DIRECTORIES.md) / [RU](../ide/dis/DIRECTORIES.ru.md).

## Previous checkpoint

# Filesystem ports and incoming heap validation — v0.1650

Current pin: **ff21d3ceaa3a0d19aa9d1359bb2d1a71a89678ef**.
Exact selections: **636 core/reference/library files + 207 bootstrap files**.
Incoming 90caf9d validates construction/clone spans before metadata/owner access.
The original heap-preflight, clone and packed-record regressions are retained.
Hosted filesystem ports remain FlashVM-specific. Reverse commit **ff21d3c**
corrects the aliased nil/H branch oracle and missing arithmetic/CALL fixture
dependencies. Actual production emitters are unchanged. The local parser fixture
also now extracts the production signed-word decoder.
[EN filesystem](runtime/FILESYSTEM.md) · [RU](runtime/FILESYSTEM.ru.md).

## Previous checkpoint: module review, reverse Regex fix and bootstrap optimizer refresh

Final pin: **320e6581e425c014f8a9ba313352b664925a709f**.
Exact selections: **633 core/reference/library files + 207 bootstrap files**.
Incoming cfb91bc bounds optimizer graph stack use and widens bitmap use/def
counters. Its graph and WORD32/64 bitmap regressions are retained and registered.

The module review reproduced a genuine Unicode collision in Regex: internal
tags and the input sentinel overlapped U+10000…U+1000B. The fix was tested and
published back to Inferno as **320e658** with bhgv author/committer, then imported
exactly. Regex is now built as an ordinary Limbo library in native and standalone
library configurations; runtime/editor SDKs include source, header, bytecode and
symbols. Relocated SDK checks compile the actual library/client and execute both
modes at quanta 1/2048. [Review](MODULE_AUDIT.md) ·
[Upstream regression](upstream/appl/lib/tests/README.regex-unicode.md).

The unmodified preceding library fails at U+10000; the fixed library passes 85
Unicode literal/capture/class checks in each of four runs. This is not a new
regexp-slice frontend syntax, dedicated regexp JIT or completed /prog debugger.
Compiler 0.47 one-name iteration remains a separate FlashVM Coco/R extension.
Native host execution is AMD64; physical ARM64/full standard ABI acceptance
remains separate. Both snapshots include the earlier reverse fixes.

---

# ALT bounds and communication-error registration cleanup

Final pin: **1bd39b084006caabda0b712d3b011975c5241814**.
Exact selections: **623 core/reference + 206 bootstrap files**. ALT/NBALT reject
wrapping fixed headers and incomplete result WORD spans before registration.
A protected selected transfer cancels this alt's registrations on error before
rethrowing, preserving unrelated waiters and FIFO order. Already performed
payload operations, peer wakeups and selection/index stores are not rolled back.
[Upstream contract](upstream/libinterp/tests/README.alt-power.md).

Owning ASan/UBSan NDEBUG matrices passed **243,860 direct-C + 243,860 complete-JIT
queue cases**, plus **599,408 payload transfer cases**. A registered core test
now repeats the complete ALT/NBALT native-tail route on AMD64/ARM64 hosts.
The full 183-test runtime pass used 9283015 with compiler 0.46; this late import
is rebuilt and tested separately for channels, errors, scalar compilation and
generation freshness. Prior reverse fixes remain upstream; no new reverse defect
was found. The optimizer change stays FlashVM-only. Native host testing is
AMD64; physical ARM64 and the full native ABI remain separate work.

---

# Captured stack teardown roots and packed diagnostic descriptors

Tested Inferno experiment pin: **92830151d9b130e6fea1f122c688165eee743fea**.
Exact selections remain **622 core/reference + 206 bootstrap files**. Stack
teardown captures each extension parent and root Type before mapped-owner
destructors run. Packed Type pointer counts and diagnostic bitmap reads use
byte-safe accesses. A destructor changing saved EX/TR cannot redirect or
truncate the remaining cleanup or suppress root-frame owners.

[Upstream contract](upstream/libinterp/tests/README.stack-frame-contract.md).
The owning WORD32/WORD64 native cleanup matrix passed **196,608 cases** under
ASan/UBSan, NDEBUG and strict warnings. This does not establish a general mutable
Type/callback contract or physical ARM64 acceptance. Prior reverse fixes remain
upstream; no new reverse defect was found. Compiler 0.46 private scalar-store
optimization remains FlashVM-only.

---

# Exact JIT cache binding and code-image refresh

Final tested pin: **6c468ef5ecf203c2ca4b0e7eef435a145399b3d0**.
Exact snapshots: **622 core/reference + 206 bootstrap files**. Reused decoded-leaf
caches must retain canonical opcode entry/tail order, exact emitted bytes, size
and compiler callback identities. An in-range entry for the wrong opcode no
longer passes reuse. Validation uses bounded private staging on cache reuse,
not on each executed instruction.
[Upstream contract](upstream/libinterp/tests/README.cache-bindings.md).

Owning ASan/UBSan native WORD32/64 dispatch/cache/integration and ARM64 host-side
compiler matrices pass. The full 179-test runtime pass used the preceding
aligned-owner snapshot c2a08e3 with compiler 0.44; after this late import the
runtime is rebuilt and targeted dispatch/ownership/compiler checks are rerun.
Prior reverse fixes remain upstream; no new reverse defect was found. The
FlashVM source extension remains outside Inferno.

---

# Heap identity alignment and priority owner JIT guards

Inferno experiment pin: **c2a08e302b766b9299a79395ce6e67302de41146**.
Exact selections: **621 core/reference + 206 bootstrap files**. MOVP, SELF,
HEADP and TAIL reject misaligned Heap identities before touching owner metadata;
packed operand slots remain supported. AMD64/ARM64 source-owner fast paths now
check alignment before retaining, marking or publishing an owner. Distinct old
destination owners still decline safely to C.

[Upstream contract and matrices](upstream/libinterp/tests/README.owner-preflight.md).
Owning native ASan/UBSan WORD32/64 and AMD64/ARM64 guest matrices pass, including
complete JIT entries and actual C fallback. No new reverse defect was found;
prior reverse fix 51d44da remains included. FlashVM compiler 0.44 deferred-step
source semantics remain outside Inferno. This does not claim completion of the
64-bit compiler/frame ABI or a physical ARM64 host run.

---

# Exception destination validation refresh

Tested Inferno experiment pin: **d229914a31f9d92bc7fb114033bcce6a869cd898**.
Exact selections contain **621 core/reference + 206 bootstrap files**. Explicit
and default exception destinations are validated before diagnostics, frame
unwinding or owner publication. Invalid destinations do not hide later valid
handlers. Interpreted targets require a complete instruction; compiled targets
retain byte offsets and reject address wrap/H without claiming an allocation
extent that the module does not record.

The owning-repository emu/os WORD32/WORD64 fixtures pass **2,104,000 cases** under
ASan/UBSan with NDEBUG. [Contract and commands](upstream/libinterp/tests/README.exception-frames.md).
The earlier reverse fix **51d44da** remains upstream; no additional reverse defect
was found in this refresh. Compiler 0.43 source initialization semantics remain
FlashVM-only. The complete 179-test runtime pass precedes this refresh; the
new snapshot is rebuilt and checked separately for exception/publication,
generated-runtime freshness and compiler execution paths.

---

# Final RAISE publication refresh

Final tested pin: **4c94e2af2e8e2a927fdbb06c800ce5dd6453784f**, including incoming
RAISE metadata preflight and safe exception-owner transfer in emu/os. Exact
snapshots contain **620 core/reference + 206 bootstrap files**.
[Exception contract](upstream/libinterp/tests/README.exception-publication.md).
The reverse diagnostic fix **51d44da** remains included and acknowledged upstream.
This refresh changes no FlashVM language/FIR semantics.

---

# Direct target synchronization and reverse diagnostic fix

Inferno experiment is pinned to **51d44da4670567c7d38820ca0b1dfd8e3e4d895d**.
Snapshots: **617 core/reference + 206 bootstrap files**. Incoming CASE/GOTO/MOVPC
span checks and direct-branch nil/H checks cover shared C and priority emitters.
[PC boundaries](upstream/libinterp/tests/README.pc-target-boundaries.md) ·
[Direct branches](upstream/libinterp/tests/README.direct-targets.md).

A reproduced native diagnostic compile failure was fixed in the owning Inferno
repository and published back as **51d44da**: leaf64-runtime borrows production
JMP/branchpc with branchreal. Native WORD32/WORD64 UBSan matrices and the refreshed
core build pass. The new native/guest target drivers and readiness checks pass.
FlashVM language/FIR initialization changes remain outside Inferno.

---

# Packed GC map synchronization

Inferno experiment is pinned to **381ecd2f8162bc02d9bda6cedfde9aad9fb8bbe6**.
Snapshots contain **610 core/reference + 206 bootstrap files**. Packed Type maps
and record slots now use byte-safe reads in GC/owner cleanup; saved RET frames
are span-checked in C and AMD64/ARM64 emitters. New GC graph/map and real builtin
cleanup regressions are retained exactly.
[Upstream contract](upstream/libinterp/tests/README.gc-packed-maps.md).
Selected GC/map/module/control-transfer/readiness checks and hosted reference
regressions passed. No new shared reverse fix was confirmed.

---

# CALL/MCALL target synchronization

Pinned Inferno experiment: **51227cb0629bc6fd755d4d6ecad44f055ea56690**.
Snapshots: **607 core/reference + 206 bootstrap files**. Incoming C and AMD64/ARM64
fast-path fixes validate CALL/MCALL bytecode target spans before publication,
preserve opaque legacy compiled PC tokens and validate emitter layouts.
[Contract and regressions](upstream/libinterp/tests/README.call-targets.md).
Native control-transfer, priority-architecture guest and readiness checks passed.
No new shared outgoing fix was confirmed; FlashVM source extensions stay separate.

---

# Module transitions and compiler 0.39

Inferno `experiment` is pinned to **45370abd78dd8b9339c962a544d2a933663139f1**.
The selected snapshots contain **606 core/reference + 206 bootstrap files**.
Incoming shared fixes capture LOAD destinations/module context across callbacks,
restore builtin MCALL cleanup from its captured frame and reject malformed
SPAWN/MSPAWN operands before allocation/publication. New upstream native C/RX
regressions are retained byte-for-byte and registered in core CTest.
[Upstream contract](upstream/libinterp/tests/README.module-transitions.md).

The outgoing comparison found no new shared defect requiring a reverse commit.
Compiler 0.39's int/bool cells, int postfix updates and string/key-snapshot
for-in remain FlashVM-only source features, using existing FIR/Dis operations.
[English](../fir/v1/doc/TYPED_LOOPS.md) / [Русский](../fir/v1/doc/TYPED_LOOPS.ru.md).
Native regression execution here is on the available AMD64 host; retaining ARM64
compiler/tail tests does not claim a complete native VM ABI port.

---

# Host extensions / Расширения хоста — v0.1649

Pinned Inferno remains **637d618e40fb6efb69cb0c147efb81981ee95c63**, unchanged.
Relative indexes and inclusive negative-end slices are FlashVM host/compiler
extensions. Source transformations retain exact checked anchors; upstream files
remain byte-exact. Snapshots: 560 core/reference + 174 bootstrap files.
[EN](../fir/v1/doc/RELATIVE_SEQUENCES.md) / [RU](../fir/v1/doc/RELATIVE_SEQUENCES.ru.md).

Broker metadata synchronization is not payload synchronization. The requested
same-operation reentrant lock is specified in [the contract](OWNED_OPERATIONS.md),
but is not claimed as implemented. Current VM ownership serializes C operations.

---

# Synchronization / Синхронизация — v0.1648

Checked Inferno experiment at **637d618e40fb6efb69cb0c147efb81981ee95c63**.
No new incoming commit followed the previous series. Earlier reverse fixes are
retained upstream. Exact snapshots: **560 core/reference + 174 bootstrap files**.
AMD64/ARM64 availability remains **166/173 guarded fast paths** and **173/173
complete decoded entries**; neither metric represents full operand semantics.

Compiler 0.31 adds hybrid table constructors through existing FIR and hosted
table services. Reused C99 source services recover after syntax/SSA-limit failures.
These language changes remain FlashVM-only; no upstream opcode, helper ABI or
container representation changes. Shared complete-dispatch, queue and readiness
checks passed. No new shared defect requiring a reverse patch was confirmed.

Новых входящих коммитов нет; прежние обратные исправления присутствуют.
Конструкторы и работа с их исходными позициями остаются в FlashVM.
[EN](../fir/v1/doc/TABLE_LITERALS.md) / [RU](../fir/v1/doc/TABLE_LITERALS.ru.md).

---

# Synchronization / Синхронизация — v0.1647

Final Inferno experiment: **637d618e40fb6efb69cb0c147efb81981ee95c63**.
The branch initially remained at 5b9bdcf; a final head check found 637d618 during
publication. It was imported and tested before completing this series.
Snapshots now retain **560 core/reference + 174 bootstrap files**.

AMD64/ARM64 retain **166/173 guarded native fast paths** and now provide
**173/173 complete decoded entries** through safe C tail bridges. These are
separate availability metrics, not complete semantic coverage or block JIT.
The imported shared executor, compiler span/cache checks and W^X publication
remain byte-exact upstream. Earlier reverse fixes are retained in experiment.

Our NC64 host routes both direct and complete fast callbacks through one policy:
classic CASE/CASEL/GOTO leaves decline into the NC64 C adapters, and only actual
fast successes increment native statistics. No Jit pointer is reread after a
callback. A standalone policy check covers both routes and negative nonzero
completion. This host-only adaptation is not sent to Inferno.

Входящий 637d618 импортирован; прежние обратные исправления сохранены.
Расширения языка и NC64-политика хоста остаются в FlashVM. Нового общего дефекта
для обратной правки не подтверждено. CVTCW/strtol Inferno не меняется.
[Compiler EN](../fir/v1/doc/TEXT_VALUES.md) / [RU](../fir/v1/doc/TEXT_VALUES.ru.md).
[Complete dispatch contract](upstream/libinterp/tests/README.complete-dispatch.md).

---

# Synchronization / Синхронизация — v0.1646

Checked Inferno experiment at **5b9bdcfd188cd16846f8935b5c7d9074dcc89ee5**.
No incoming commit followed the previous series. Its reverse LeafLayout fix
remains published upstream. Exact snapshots: **554 core/reference + 174 bootstrap**.
Native availability remains AMD64/ARM64 166/173 with partial operand coverage.

Compiler 0.29 adds checked signed32 shifts in FlashVM-only Coco/R/FIR lowering,
reference execution and both direct Dis emitters. It uses existing branch/shift
kernels and their exception path; there is no upstream VM opcode or ABI change.
Selected shared shift-slot, native WORD32/64 and readiness checks passed.
No new shared defect requiring another reverse patch was confirmed.

Входящих изменений нет; прежнее обратное исправление остаётся в experiment.
Новые операции FIR и расширение исходного Limbo остаются в FlashVM.
[EN](../fir/v1/doc/INT_SHIFTS.md) / [RU](../fir/v1/doc/INT_SHIFTS.ru.md).

---

# Synchronization / Синхронизация — v0.1645

Inferno experiment: **5b9bdcfd188cd16846f8935b5c7d9074dcc89ee5**.
No new incoming commit followed 6945acaf before this series. Expanded string JIT
checks found a shared fixture ABI drift: Python LeafLayout lacked the buffered
channeltransferentry callback present in C. The strict C size/offset checks
correctly stopped execution before code generation.

[Reverse fix 5b9bdcf](https://github.com/bhgv/Inferno-OS-bhgv/commit/5b9bdcfd188cd16846f8935b5c7d9074dcc89ee5)
adds the missing field in its exact position. Size/offset checks remain enabled;
no production opcode, ABI or acceptance guard was weakened. The fix was tested
in its owning Inferno checkout before publication and imported back unchanged.

В Inferno отправлено исправление описания структуры ctypes; производственный
код JIT не изменён. Снимки: 554 core/reference + 174 bootstrap файла.
Availability remains 166/173 per AMD64/ARM64, with partial operand coverage.
Compiler 0.28 string expressions and int bit operations remain FlashVM-only.
[EN](../fir/v1/doc/TEXT_BITS.md) / [RU](../fir/v1/doc/TEXT_BITS.ru.md).

---

# Synchronization / Синхронизация — v0.1644

Inferno experiment: **6945acafa0df51c7ee3e591b1665ddb40d9a27bb**.
Imported 87785ec: native buffered SEND/RECV for AMD64 and ARM64 with guarded
fallbacks. Exact snapshots: **554 core/reference + 174 bootstrap files**.
Native availability: **166/173** per architecture; this is partial operand
coverage, not a semantic coverage percentage.

Two fixes returned upstream / Два исправления отправлены обратно:
- [046bb1f](https://github.com/bhgv/Inferno-OS-bhgv/commit/046bb1f7e90619e421a7c23e95025500d343521c): readiness checks no longer write fixture bytecode caches.
- [6945acaf](https://github.com/bhgv/Inferno-OS-bhgv/commit/6945acafa0df51c7ee3e591b1665ddb40d9a27bb): queue tests extract the real channel span/peer guards.

Compiler 0.27 int arithmetic remains FlashVM-only, including table-profile
extensions. Расширения языка в Inferno не переносятся.
[EN](../fir/v1/doc/INT_ARITHMETIC.md) / [RU](../fir/v1/doc/INT_ARITHMETIC.ru.md).

---

# Current synchronization — v0.1643

Experiment remains dd0f5d6440de8364a182c8c8eca0976d63e7d485. Both directions
were checked; no incoming commit followed the prior series and earlier reverse
fixes remain in history. Snapshot verification retains 545 core/reference and
174 bootstrap files. No shared native defect requiring a new reverse patch
was confirmed; native source and module ABI are unchanged.

Compiler 0.26 adds Unicode string len/index/slices through three append-only
FIR operations and existing Dis LENC/INDC/SLICEC. String lengths/indexes/results
use int; hybrid table/key lengths remain big. Strict UTF-8 validation now also
covers caller-constructed live string constants in the C99 FIR API.
These frontend/FIR fixes remain FlashVM-only.
[Contract and tutorial](../fir/v1/doc/UNICODE_STRINGS.md).
Native availability remains AMD64/ARM64 164/173 with partial operand coverage.
This series validates generated host Dis, not physical ARM64/full Inferno boot.

---

# Current synchronization — v0.1642

Experiment dd0f5d6440de8364a182c8c8eca0976d63e7d485 is imported byte-for-byte:
545 core/reference and 174 bootstrap files. Incoming mapped VM/channel copy
preflight validates operand slots, complete metadata/maps and payload spans
before ownership changes or publication. Zero-size records retain nonaccess;
AMD64/ARM64 retain the pointer-free 0..256 native domain and C fallback.
[Owning contract](upstream/libinterp/tests/README.mapped-preflight.md).
Two new native/emulation drivers are registered. Earlier reverse fixes remain
in experiment history; no new shared native defect was confirmed during import.

Compiler 0.25 compound hybrid-table updates remain FlashVM-only. There are no
new FIR IDs/types, native opcodes or helper/container changes in this frontend
extension. Locations and old values are captured once before RHS evaluation.
[Contract](../fir/v1/doc/COMPOUND_UPDATES.md).
Fresh relocated SDK acceptance now also relinks an in-process source+FIR C99
client and runs every branching tutorial through it, without compiler subprocesses.
Native availability remains AMD64/ARM64 164/173 with partial operand coverage;
instruction emulation is not hardware/full Inferno boot evidence.

---

# Current synchronization — v0.1641

Experiment was checked in both directions at b10f81fa764403adc1321a77136ecfe06e07cb8a.
There are no new incoming commits after the previous series. The 541 core/reference
and 174 bootstrap files remain exact. Earlier reverse branch/JMP fixes are already
in experiment history; no new common native defect was confirmed in this series.
No duplicate synchronization commit is created in Inferno.

Compiler 0.24 adds lazy logical expressions with three append-only FIR operations
and a function-local scalar bool-cell type. Dis lowering uses existing MOVW;
there is no new VM opcode, module helper, GC pointer or heap-crossing ABI.
These language/FIR extensions remain FlashVM-only.
[Contract and tutorial](../fir/v1/doc/SHORT_CIRCUIT.md).
Native availability remains AMD64/ARM64 164/173 with partial operand coverage.
This series validates compiler-generated Dis on the host, not a physical ARM64
port or a full Inferno boot.

---

# Current synchronization — v0.1640

Experiment `b10f81fa764403adc1321a77136ecfe06e07cb8a`. 541 exact core/reference files and 174 bootstrap files.
Incoming b10f81f guards INDB/INDW/INDL/INDF/INDX/LENA and shares overlap-safe
qword/byte copy kernels across MOVM/HEADM/MOVMP/HEADMP on AMD64/ARM64. Descriptor,
map and pointer-free 0..256-byte native limits remain; larger records use C.
[Owning contract](upstream/libinterp/tests/README.array-slots-copy-kernel.md).
Three new native/emulation drivers are registered. Earlier reverse branch/JMP
repairs are present in this experiment history. No additional shared native
fault requiring a new reverse commit was confirmed during this synchronization.

Compiler 0.23 comparisons-as-values and logical negation remain FlashVM-only.
Their precedence was checked against Inferno limbo/limbo.y; there is no new
native opcode, module helper ABI or file-container tag.
[Compiler guide](../fir/v1/doc/COMPARISON_VALUES.md).
Native availability remains AMD64/ARM64 164/173 with partial operand coverage.
ARM64 instruction emulation does not establish hardware execution/full OS boot.

---

# Current synchronization — v0.1639

Experiment `80f859e`: exact fixed operand/frame guards and AMD64/ARM64 qword
MOVM/HEADM kernels were imported from their owning Inferno commit. Native copies
retain the 0..256 domain and C fallback for larger records. Backward/forward
blocks and 0..7-byte tails preserve overlap order; no empty payload is accessed.
The three new C/native-emulation drivers are registered in our CTest selection.
[Upstream contract](upstream/libinterp/tests/README.fixed-slots-copy-chunks.md).

No additional owning native defect was confirmed during this import. Our earlier
reverse branch/JMP repair remains in experiment; no duplicate reverse commit was
created. Compiler 0.22 lexical arm/scoping extensions remain FlashVM-only.
[Compiler guide](../fir/v1/doc/TERMINAL_ARMS.md).

AMD64/ARM64 availability remains 164/173, with partial operand coverage. Emulated
ARM64 checks do not establish hardware execution or full Inferno boot.

---

# Current synchronization — v0.1638

Experiment: `02b7b7f96183d30f6c75b7415197ecc986e5d411`. 531 exact core/reference
files and 174 bootstrap files. Our reverse integer-branch/JMP preflight is
published in experiment and imported byte-for-byte. It bounds used comparison
and target slots, retains false-target/JMP-source nonaccess, computes the ARM64
inverse relocation and preserves native FP/PC state on decline.
[Owning contract](upstream/libinterp/tests/README.branch-slot.md).

New countof/in/delete language operations, FIR IDs and hosted Bigtable helpers
are FlashVM extensions and were not sent to Inferno. [Compiler contract](../fir/v1/doc/TABLE_MUTATION.md).
The real ide.dis application entry runs Limbo policy on this DisVM and FlashVM
rendering through typed channels. [Application guide](../ide/dis/app/README.md).

Native availability remains AMD64/ARM64 164/173, with partial operand coverage.
No native ISA/container/WORD ABI change is introduced. ARM64 checks are instruction
emulation; hardware execution and full Inferno boot remain separate acceptance.

Incoming e14f034 adds independent integer branch C/RX and JIT checkpoints.
Incoming 02b7b7f guards nine numeric commands and accepts all normal exact
CVTFX power scales. All four additional native/emulation drivers are registered.

---

# Current synchronization — v0.1637

Experiment: `14c3d2c2736936eade4eadfa25e87c8ce5e404ec`. 519 exact core/reference files and 174 bootstrap
files. Incoming 7254e9e checks eighteen transfer/conversion paths; 2190eea checks
24 integer ALU/division paths. Their production implementations are retained.
Our initially independent division repair was not published over the concurrent,
broader 2190eea implementation.

Our reverse 14c3d2c adds six real-branch source/conditional-target guards and
independent twelve-command division/real checks. False predicates ignore targets;
taken targets fit before PC publication. ARM64 relocates the inverse branch over
the complete guarded sequence. The same reverse commit preserves transfercheck
and integercheck when standalone numeric oracles are embedded in native WORD
model diagnostics. [Arithmetic contract](upstream/libinterp/tests/README.arithmetic-slot.md).
[Transfer](upstream/libinterp/tests/README.transfer-slot-preflight.md) ·
[Integer ALU](upstream/libinterp/tests/README.integer-slot-preflight.md).

All three independent checkpoint families are registered: 190 selected DisVM
tests. [New commented Limbo example](../fir/v1/examples/tables/division.b) covers
MIN/-1, signed remainder and an unselected zero-divisor arm. SDK acceptance uses
fresh C99 compiler/reference clients, original/optimized FIR and both VM modes.

Native availability stays AMD64/ARM64 164/173; operand coverage is partial. Native
ABI/private capacity stay unchanged. Arithmetic spans do not prove arbitrary
mapped allocation ownership. ARM64 evidence is instruction emulation. Compiler
language extensions remain in FlashVM; no full Inferno boot/hardware claim.

---

# Current synchronization — v0.1636

Experiment: `d899383090f6013e8be41da7e3e2c09888856b0b`. 507 exact core/reference files and 174 bootstrap
files. No new incoming commit followed 9de5fe9 at the initial synchronization.
This series returns our confirmed decoded-slot fix for eight integer shifts to
Inferno and imports the same committed sources without adapter divergence.

SHLB/SHRB/SHLW/SHRW/LSRW/SHLL/SHRL/LSRL validate the native count-slot span
before loading WORD. Invalid count values keep priority over value/result errors.
For valid counts both remaining spans fit before the value read. C raises bounds
for malformed slots; AMD64/ARM64 decline without stores. All operands precede the
possibly overlapping result publication. Native pointer width stays separate
from WORD32/WORD64. [Contract](upstream/libinterp/tests/README.shift-slot.md).

New registered C/native-emulation checkpoints cover 2,944 error/atomicity cases
and 51,200 image/read/write/count/FP cases. All 24 previous-version command and
backend probes reproduce the original null count-slot read. Existing integer
and shift/division fixtures retain their arithmetic/alias coverage and now
supply the production bounds-error declarations for extraction.

Availability remains AMD64/ARM64 164/173 active opcodes, with partial operand
coverage. Arithmetic spans cannot prove arbitrary mapped allocation ownership.
ARM64 instruction emulation is separate from physical ARM64/full Inferno boot.
No language-extension export or native WORD ABI change is part of this series.

---

# Current synchronization — v0.1635

Experiment: `9de5fe916f66efbafa91a7661a82b0b67f2272d3`. 503 exact core/reference
files and 174 bootstrap files. Incoming 808a4f6 and 70611aa repair C/AMD64/ARM64
string predicate and owner preflight: conditional branch targets remain unread
on false predicates; source lifetime errors precede destination-owner reads and
reference/GC publication. SELF ignores its unused source slot.

Our reverse commit ca53cc6 registers an explicit optimized-Python owner checkpoint
in Inferno. It forces both architectures and clears interactive family filters.
FlashVM now registers both full owner emulation and the optimized checkpoint in
its CMake test suite; the upstream fixture was previously absent there.
[Owner contract](upstream/libinterp/tests/README.owner-preflight.md) ·
[String branch contract](upstream/libinterp/tests/README.string-branch-preflight.md).

Native availability remains 164/173 on AMD64 and ARM64. Arithmetic pointer-span
checks do not establish mapped allocation ownership. ARM64 instruction emulation
is not physical ARM64 execution or a full Inferno boot. Compiler table/any/int
extensions were not exported; the native WORD ABI remains unchanged.

The final follow-up 85cd4e5 additionally imports decoded-slot preflight for
HEADB/HEADW/HEADL/HEADF/LENL/HEADM/MOVM/TCMP. It preserves zero-copy nonaccess,
source/count error ordering and write-free native decline. Native and emulated
list-slot checkpoints are registered in our CMake suite. Upstream explicitly
validated our incoming optimized owner checkpoint.
[List/copy/type contract](upstream/libinterp/tests/README.list-slot-preflight.md).

Our second reverse commit 9de5fe9 repairs the raw-copy regression fixture's stale
64-unit limit after ARM64 HEADM guards grew. The fixture now uses the existing
128-unit private capacity, checks every insufficient size, exact output and a
trailing canary, and invalidates the complete code window. Production capacity
and ABI are unchanged. All 411,264 differential copy cases pass under Python -O
and UBSan across AMD64/ARM64/RV64.

---

# Current synchronization — v0.1634

Experiment: `cd402a0ac5a8a2a8b18d3d60d6229e4bc2788a26`. 496 exact core/reference files and 174 bootstrap files.
Imported SLICEC/LENC C/AMD64/ARM64 preflight validates decoded slots, required
metadata and backward reference cells before access. Invalid end geometry
rejects before owner reads; zero-start prefixes keep metadata-only behavior.
Native decline remains write-free. [Contract](upstream/libinterp/tests/README.slice-length-preflight.md).

The incoming commit explicitly integrates our previous reverse commits 9f00116
and 826dae5. Their independent INDC and private-capacity tests remain selected.
No new upstream-owning defect was confirmed for reverse publication. Our typed
int/big-shift compiler extension remains outside Inferno; the native WORD ABI
is unchanged. AMD64/ARM64 availability remains 164/173 active guarded opcodes,
not complete operand coverage or physical ARM64/full Inferno boot acceptance.
Historical reports follow.

---

# Current synchronization — v0.1633

Experiment: `826dae5c387528f3120728f0bf61204b76639658`. 495 exact core/reference files and 174 bootstrap files.
Incoming ADDC, decimal and INDC/INSC repairs preflight decoded slots, String
headers and backward reference cells before reads/publication on C/AMD64/ARM64.
Native paths retain write-free decline; availability remains 164/173 active
opcodes, not complete operand coverage.

The reverse publication is 9f00116: our independent C/native INDC regressions,
checkpoint registration and expanded private-capacity sweeps. Upstream b6c160b
landed a concurrent, broader INDC/INSC implementation while our expected-head
publication was in progress. That implementation was preserved; the earlier
local functional commits were not published. The returned tests pass against
b6c160b. [Independent contract](upstream/libinterp/tests/README.indc-preflight.md) ·
[Owning repair](upstream/libinterp/tests/README.string-cell-preflight.md) ·
[Decimal](upstream/libinterp/tests/README.decimal-preflight.md) ·
[ADDC](upstream/libinterp/tests/README.append-preflight.md).

Our acyclic table CFG remains a local compiler/runtime capability; no table
language extension is exported to Inferno. AMD64 RX and ARM64 instruction
emulation remain distinct from physical ARM64/full Inferno boot acceptance.
The second reverse publication, 826dae5, fixes the adjacent decimal-span driver's stale 64-unit buffer and verifies NULL sizing plus every insufficient capacity
against production's existing 128-unit limit. Historical reports follow.

---

# Current synchronization — v0.1632

Experiment: `583c3ce6ea8629aa37a1e0237a2b8d99f4a39004`. 488 exact core/reference files and 174 bootstrap files.
AMD64/ARM64 CASEC accepts guarded lower-label identity/equal-empty matches at
the first binary-search midpoint in tables up to 4095 rows. Other comparisons
retain C fallback; availability stays 164/173 active native opcodes.

Production C string comparison/duplication now guards wrapping String headers,
heap-owner underflow and complete ASCII/Rune source spans before field reads,
allocation or copying. The new string-header/source regression is selected and
registered alongside existing native RX/emulation tests.
[Contract](upstream/libinterp/tests/README.string-case-native.md).

The complete core passes 176/176; runtime/IDE passes 117/117. AMD64 native RX
execution and ARM64 instruction emulation remain separate from physical ARM64
acceptance/full Inferno boot. No new upstream-owning defect was found for reverse
publication. Our big comparison/source decision extension stays outside Inferno.
Historical synchronization reports follow.

---

# Current synchronization — v0.1631

Experiment: `a3fb27ec812fd0c7d544451e2a003d6a4bf84034`. 486 exact core/reference files and 174 bootstrap files.
AMD64/ARM64 CASEC validates the complete declared count-cell span before reading
it and rejects narrow nonempty cell4 tables before loading rows/default targets.
Single-row dispatch now also accepts distinct empty String owners and H/empty
pairs after checked length/capacity metadata. Availability stays 164/173 active
opcodes: this extends guarded input coverage, not a complete native string search.

The bootstrap retains exact big case assembly cell padding, NaN payloads and
quoted data/source paths. Both production Str copies receive radix/digit and
decimal exponent fixes. New acceptance drivers retain the production assembly
serializer and both Str integer bodies; they are not a full assembler or a
self-hosted Limbo module run. [Assembly](upstream/limbo/tests/README.asm-roundtrip.md) ·
[Str](upstream/appl/lib/tests/README.string-radix-host.md) ·
[CASEC](upstream/libinterp/tests/README.string-case-native.md).

All 175 core tests have successful results. Initial assembly-driver failure was
a missing selected g_fmt.c dependency; the final exact snapshot includes it and
the driver passes. AMD64 RX execution and ARM64 instruction emulation are distinct
from physical ARM64 acceptance. No new upstream-owning defect was identified for
reverse publication. Our FIR scalar big CFG/verifier changes are local compiler
work; no table-language extension is exported to Inferno.

---

# Current synchronization — v0.1630

Experiment: `1ae1470f6960173a2330f212ee76d32843ae2736`. 478 exact core/reference files and 174 bootstrap files.
Imported guarded AMD64/ARM64 ICASEC paths accept empty tables or exact lower-label
owner identity in one-row tables. Availability is 164/173 active opcodes, not
full native string-table search. Other inputs preserve checked C behavior.

Full-width counts, String headers, PC bounds and long decline branches are
validated before REG.PC publication. There are no runtime callbacks, allocation,
GC/refcount writes or FP changes. [Contract and evidence](upstream/libinterp/tests/README.string-case-native.md).

The complete core suite passed 173/173. Native AMD64 RX and ARM64 instruction
emulation remain separate from physical ARM64 acceptance. No new owning defect
was identified for reverse publication. The language extension stays outside
Inferno. Historical reports follow.

---

# Current synchronization — v0.1629

Experiment: `5c79135244b0c2e651d200ec3caa4b589833dc60`. 474 exact core/reference files and 174 bootstrap files.
Imported AMD64/ARM64 paths cover raw/mapped NEWCM/NEWCMP and pointer-free mapped
CONSMP. Availability is 163/173 active commands; guarded cases may retain C.
Physical ARM64 execution remains separate from instruction-emulation evidence.

The refresh includes protected string case tables, mapped-head retain rollback,
and numeric case/goto readers matching declared IBY2WD cells. Meaningful scalar
width, cell width, VM WORD and native pointer width remain independent contracts.
The first snapshot exposed five Python fixture ABI failures; upstream b579b5f
already repaired that layout. Our returned 5c79135 expands and documents the
fixture fields for review, preserving b579b5f's concurrent functional work.

[PC cells](upstream/libinterp/tests/README.pc-table-cells.md) ·
[Mapped lists](upstream/libinterp/tests/README.mapped-cons-native.md) ·
[Channels](upstream/libinterp/tests/README.payload-channel-native.md).
The complete refreshed core suite passed 171/171. No Limbo table extension was
exported to Inferno. Historical synchronization reports follow.

---

# Current synchronization — v0.1626

Priority: x86-64 and ARM64. Experiment: `875d12263efa72bab84d27f0d4215713e7da9465`.
Imported cc51c63 adds five scalar-channel constructor paths: NEWCB/NEWCW/NEWCL/
NEWCF/NEWCP. AMD64/ARM64 availability is 160 of 173 active opcodes, not complete
operand coverage. RV64 code is unchanged in this series.

Our returned TCMP fix guards backward Type-field addresses in the C interpreter
and both priority JITs. Small nonzero owners previously underflowed the address
or read the Type field at nil; UBSan reproduces the defect against the parent.
Native decline preserves the VM image and performs no ownership/GC changes.
The generic emulation fixture also includes the new channelentry ABI field.

[Scalar channels](upstream/libinterp/tests/README.scalar-channel-native.md) ·
[TCMP contract](upstream/libinterp/tests/README.typecheck-prefix.md).
453 exact core/reference files and 174 bootstrap files are selected. No table
language extension was exported to Inferno. ARM64 instruction emulation does not
establish physical hardware execution. Historical reports follow.

---

# Current synchronization — v0.1625

Selected experiment: `b2da16226dfc5bc0e0c754ae5d95fcde6cc1afb9` (three commits after b9a569e).
The Dis snapshot contains 446 byte-identical files; the temporary bootstrap has 174.

AMD64/ARM64 now include native ICONSM/ICONSP list payload construction. Raw
copies stage destination ownership until allocation and copying succeed; pointer
payloads retain and mark current owners after the allocator returns. Copies above
256 bytes preserve the checked interpreter fallback. RV64 availability is unchanged.
Strict heap-prefix guards reject the exact boundary that would access address zero.
See [payload ownership and test contracts](upstream/libinterp/tests/README.payload-cons-native.md).

Compiler synchronization includes bounded source-history growth and overflow-safe
coordinate/case lookup, with production fixed-point selection and cast/string
folding regressions. These are imported compiler references and bootstrap fixes;
our Coco/R frontend and its native table Dis backend remain independent.

Seven acceptance drivers are now registered: payload layout, AMD64 native payload,
AMD64/ARM64 emulation, source history, fixed selection, cast folding and string folding.
WORD32/WORD64 layout models and ARM64 instruction emulation do not establish a
physical ARM64 port. No table-language extension is exported to Inferno.

Earlier synchronization reports follow as history.

---

# Current synchronization / Текущая синхронизация — v0.1593

Selected experiment: `74c193724a7352b8fda2724a5cf10029885be68c`.
Includes unique array-view SLICEA leaves and self-load ownership from 4d9ce2e,
plus our returned Loader construction/scalar result-slot fix.
[Module audit](libraries/MODULE_AUDIT.md) · [Upstream Loader contract](upstream/libinterp/tests/README.loader-results.md).
Vendored files remain byte-identical; native ARM64/RISC-V platform execution
is not established by host/emulator fixtures. Earlier reports follow as history.

---

# Dis, JIT and Sys refresh — v0.1540

[Полный русский отчёт](SYNC.ru.md) · [Manifest](UPSTREAM.json)

Source: https://github.com/bhgv/Inferno-OS-bhgv, **experiment**.
Pinned commit: `f948343b711c8300dbf76417254c18a7a8d2f5ff`, eight commits after
previous Dis snapshot `24892373ee09a2a23dc39b9dedc01fb7af61d8c1`.

## Imported and adapted

AMD64/ARM64 real arithmetic, powers/conversions/rounding, signed fixed scales
and minus-one division; emu/os Sys FD ownership/publication and unaligned
connection/result slots; process/heap diagnostics. The native Limbo bootstrap
also retains divisor evaluation and division-by-zero traps when fixed scaling
would otherwise lower to a constant zero.

The selected snapshots contain 192 Dis/reference-test files and 174 temporary
bootstrap files. Original blobs, notices and SHA-256 hashes are retained.
The new devprog bodies and two compiler source files support extracted
regressions only. No whole Inferno, GUI, Math or Geometry module was imported.
The project's Coco/R compiler remains the priority.

CMake stays compatible with 3.10; the Dis component version is now 0.1.4.
Eleven new production regressions and eight foreign-emitter build checks are
registered. TLS register/safepoint generation and the FlashVM owner bridge retain
their ABI. The Sys module header is unchanged; its implementation was refreshed.

## Executed validation, Linux AMD64

- Core build and regressions: 42/42.
- TLS/FlashVM bridge/GC tests plus foreign-emitter checks: 62/62.
- Eight focused Sys/diagnostic drivers with UBSan: 8/8.
- Source-built Limbo bootstrap product test: 1/1.
- Six IDE service modules and their .sbl files rebuilt with that bootstrap.
- Both provenance manifests verified.

Native AMD64 execution, WORD-width fixtures and ARM64/RV64 emitter construction
are distinct evidence. No ARM64/RV64 execution or Windows/macOS/Android runtime
acceptance is claimed. Sys tests inject OS/scheduler boundaries around actual
production bodies. The complete executable Dis host, Sys/VFS registration and
IDE startup remain unfinished. No newly confirmed upstream defect was found.

See the Russian report for source selection, repeat commands and integration limits.
