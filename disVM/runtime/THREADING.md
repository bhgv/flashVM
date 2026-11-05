## Default console profile

On Linux AMD64, console DisVM starts two managed executors and native GC when
no explicit execution profile is selected. The default enables guarded copies
and restricted shared admission for typed pointer, record, byte, slice and
channel transfers. Other instructions continue to use exclusive admission.
Use `--gc-inline` or `--managed-workers 1` for a single-executor run. Explicit
GC and experimental switches select a custom profile. On this console backend,
`--managed-workers 24` keeps the parallel profile and starts 24 executors.
`--managed-workers auto` starts with two executors and can grow to compensate
for native waits when guest work is runnable. An optional
`--managed-workers-limit N` caps that growth. Guest channels and runnable
processes are multiplexed over the executor pool. The
graphical host and other backends keep their existing launch defaults.

[Runtime0.89: opt-in guarded scalar/raw copies and checked SELF](SHARED-BYTES.md).
NativeWorld remains required; the flag is not parallel heap activation.

[Runtime0.88: actual MOVP retain/publication under allocation-free range guards](SHARED-POINTER.md).
Other operations and general parallel managed admission remain unfinished.

[Runtime0.87: canonical per-object guards tied to real rooted tickets](GUARD-REGISTRY.md).
This establishes identity/lifecycle, not parallel managed payload permission.

[Runtime0.86: atomic GC work consumption, history and actual collector tests](GC-PENDING.md).
General managed admission remains serialized; pending exchange alone does not enable parallel epochs.

# Native UI and Dis execution owners

Runtime0.40.0 / IDE0.7.16 / workspace v0.1732.
[Русский](THREADING.ru.md) · [Runnable managed example](https://github.com/bhgv/flashVM/blob/master/disVM/examples/threaded-owner/README.md).

`disvm-editor` now runs the complete Dis interpreter/JIT, loader, document
services, channels and collector on a dedicated OS thread. SDL, FlashVM objects,
input, local graphic gestures and presentation remain on the launching main
thread. Existing detached raster workers continue independently. This is the
default `--threaded-ui` mode; `--sequential-ui` selects the retained reference
execution route. Duplicate execution-mode flags are refused before launch.

## Ownership and queue boundary

| State | Exclusive owner |
| --- | --- |
| extended up, active REG, error frames, process FIFO, module/heap/GC state | Dis OS worker |
| SDL, FlashVM context, MovieClips, current scene, native live/frozen ink registry | Main/UI |
| detached raster snapshots | Existing raster workers |
| transport records | Copied bounded channel values |

`editor_thread.c` launches/join-controls the Dis entry and owns one synchronized
RPC slot. `editor_host.c` separates the Dis service pump from the UI pump;
`editor_bulk.c` forwards copied geometry to the same bounded UI queue. Main never
reads a Dis channel, Prog, stack, module or heap object. Dis never calls the UI
context, scene or ink registry directly in threaded mode.

The shared transport retains32 records per direction and the local input FIFO.
Bulk send can block its Dis producer under backpressure; main/UI keeps running.
The existing logical End/deferred-geometry fence protects partial scenes. A
queue receipt means copied transport acceptance, not completion of a document
operation. Native views keep their existing local gesture/frame-free rendering
contracts; this change separates OS scheduling from document reply latency.

Startup rows/preferences/path and ink read/ack/cancel use owner RPC. The caller
copies generated native frames before waiting. Input bytes are copied into native
storage, bounded at2MiB; UI replies with copied scalars or malloc-owned text.
Only frozen ink is returned. No native/managed address is a wire payload. The
Dis caller may wait; UI never waits on this RPC. Returned text is freed even if
installing the Dis string raises an allocation error. The original mapped frame
continues rooting the native call's arguments/result during that wait.

## Lifecycle and errors

Main parses options before thread creation. The worker starts its own extended
up/REG TLS and initializes the real runtime there. UI construction runs on main
through startup RPC. Startup/transport failures become a controlled runtime
failure and trigger owner shutdown. Closing queues rechecks synchronized stopping
state to distinguish normal EOF from a transport failure.

The Dis owner detaches managed channel roots and destroys its pumps. Its entry
returns normally, then it unbinds/destroys native TLS before publishing completion.
Main joins before freeing transport, RPC storage, UI state or borrowed arguments.
UI failures close queues so backpressured Dis sends wake. Cancellation remains
cooperative; a blocking uncooperative native callback can still delay shutdown.
Hard allocator/process exits retain their existing process-fatal semantics.

For diagnostics set `DVM_EDITOR_THREAD_STATS=1`. A normal run reports checked
worker binding/retirement, main's absent Dis TLS and main-loop poll count:

```
NATIVEOWNERS ui=main dis=worker ui_polls=... main_dis_tls=0 worker_bound=1 worker_released=1 result=0
```

Counts are observations, not a frame-rate/latency guarantee. The main thread can
serve input/native local gestures while Dis computes; ordinary commands which
need document authority still await its reply.

## Scope and validation

The extended-up/REG preparation is used directly through native_process and one
native_context TLS owner. Real managed applications, interpreter/JIT counters,
collector activity and normal returns are checked in8 separate processes; module
and UI initialization failures check retirement, and duplicate flags are rejected.
Actual desktop SVG/pixel editing and clipboard/tool replays validate the threaded
UI route. A sequential replay remains a reference. Register/world/Fast-Cycles
coordination fixtures retain their focused, separate evidence.

There is one Dis heap mutator/scheduler owner in this stage. Limbo spawn tasks
remain cooperative on it. The world/safepoint and GC-worker infrastructure remains
available for parallel shared-heap Dis scheduling, actual collector integration,
blocking Sys release/reentry, module/JIT publication and synchronized heap metadata.
These are the next runtime stages; this commit does not enable unsynchronized
multiple Dis mutators or executable AS asinc capture lowering.

Only native Linux x64 has been executed in this session. Common thread-port use
does not constitute physical ARM64/RV64, Android or desktop-display acceptance.

## Runtime0.41.0: actual native cycle collector

Workspace v0.1733 adds `--gc-worker` to both console and graphical runtimes.
`--gc-inline` selects the single-executor reference. A single collector OS thread binds
its own extended-up/REG and error environment. Coalesced scheduler requests invoke
the retained `rungc` algorithm, using its existing adaptive work bound. Eager RC
still executes on the Dis mutator; no unused deferred-RC lane is advertised.

`runtime_gc.c` connects native_world to the actual process list and heap. At a
complete-instruction safepoint, the mutator parks; the stopped-world visitor saves
its active REG into the current Prog.R before any collector scan. Between xec
calls saved Prog.R is authoritative and the TLS active-process slot is cleared.
Idle graphical scheduler turns also poll. The collector never races heap/process
list/module mutations: it owns the heap only while the Dis mutator is parked or
has left. Requests are native flags, not transferred managed pointers.

Managed normal/error returns leave the active interval, drain an accepted request,
join the collector and detach the poll hook before mutator TLS retirement. Error
formatting uses the current native owner's error buffer. A collector exception is process-fatal while the mutator stays parked; a
partially swept heap must not resume. Protocol failures latch failure. Final statistics follow join;
`NATIVEGC` records real passes, root snapshots, reclaimed objects and join status.
Hard process exits retain process-fatal semantics. Blocking native Sys callbacks
still delay rendezvous until they return; release/reentry around such work remains
open. GC and a mutator do not execute against the same heap simultaneously.

The real cyclic-garbage example and its sources ship in examples/gc-worker.
Six successful runs plus two managed-limit failures execute for each console/UI
host, across interpreter/JIT and three scheduler quanta. Tests require live-root
preservation, positive cycle reclamation/epochs/snapshots and post-join accounting.
Parallel shared-heap spawn, synchronized module/JIT publication and deferred RC
are still separate unfinished stages. Native Linux x64 is the executed target.

## Runtime0.42.0: Fast and Cycles use the actual heap

Workspace v0.1734 replaces the one-off collector launcher with the existing
production gc_workers service. Fast and Cycles are separate OS threads, each
with its own bound extended-up/REG and owner-local error buffers. Their heap
leases serialize; each pass stops the Dis mutator through native_world. A Fast
MORE result reschedules another bounded batch, alternating with pending Cycles.
They are separate workers, not simultaneous unsynchronized heap access.

The runtime-only prepared heap view delegates last-owner destroy to runtime_rc.c
when --gc-worker is enabled. Pinned upstream and standalone fixtures remain
unchanged. A queued object keeps one queue-owned reference and an external ptradd
root, with Setmark at admission. This represents zero remaining caller owners,
not an unrooted ref==0 pointer. Fast removes the root and calls retained destroy;
recursive destruction bypasses requeue because it runs on the collector context.
Native arrays/slices keep the existing live-reference validation semantics.

Admission is bounded by512 slots and4MiB of direct block payloads. Descendant
graph sizes are not included in that byte bound. A full queue, an oversized block,
or failed root registration keeps ownership unchanged and uses ordinary inline
destruction. Failed root registration restores the caller's error text. Each Fast
pass handles256 top-level owners; nested retained destructors are not preempted.
No arbitrary graph is advertised as a fixed-time pass. Final shutdown closes
admission, leaves the mutator, drains accepted Fast MORE work and Cycles requests,
then joins both workers before releasing roots/TLS. NATIVERC reports transfers,
releases, remaining owners/bytes and both fallback kinds after join.

The real managed gate now runs per host: six root/cycle-reclamation successes,
two slot-saturation successes, and two managed-error joins, across interpreter/JIT.
All successful runs exercise an oversized6MiB direct-block fallback. Existing
real graphical object/node SVG replay also exercises this host route.

### Reference publication and remaining concurrency boundary

The retained movp already retains/marks the new reference, publishes the slot,
then releases the old owner. This agrees with the proposed transfer ordering.
Setmark is the retained incremental collector barrier; it is not a complete SATB
old-reference log, atomic RC API or permission to traverse mutable C slots in
parallel. Native channel movers, typed copies, stack/module updates, array/list
operations and JIT paths must share a proven barrier protocol before concurrent
marking can be admitted. A transfer flag which is merely skipped is insufficient;
changed edges must be retained or rescanned before reclamation.

At this checkpoint queue metadata and non-atomic heap/refcounts are protected by
exclusive world ownership. Parallel shared-heap mutators, a persistent epoch/rooted
mutation journal, and a concurrent marker remain open. --gc-inline retains ordinary
immediate RC and inline cycle collection as the default reference mode.

## Runtime0.43.0: no mutex for an unrequested instruction poll

Workspace v0.1735 removes the world mutex from the normal instruction boundary.
The collector publishes an atomic stop-request word before waiting for active
owners. A mutator which observes no request continues without locking or touching
shared root snapshots. A requested stop follows the existing mutex/condition
rendezvous: publish complete roots, park, let both collectors serialize their
heap leases, then resume. The flag is a wakeup gate, not collection admission or
a managed-reference/write barrier. A collector still waits for every active owner.

The request uses GCC/Clang acquire/release builtins or MSVC Interlocked operations.
Unknown compilers conservatively use the locked route without reading a shared
plain word. This preserves C99 host compatibility. Clearing the request happens
under the world mutex before resume; the existing resuming-generation fence
prevents another collector from overtaking parked-owner retirement.

`dvm_world_create_mode(1)` retains fully locked polling. The runtime exposes it as
`--gc-worker --gc-locked-poll`; default worker polling uses the atomic gate.
`dvm_world_owner_polls` copies owner-private fast/locked counts only on that same
bound context, avoiding shared counter races. NATIVEGC reports those counts after
joins. The standalone native fixture deterministically checks exactly10000
unrequested polls in each route, inactive rejection and participant retirement,
then performs100 real root rendezvous with two mutators and separate collectors.
These focused instruction fixtures remain distinct from whole managed heap proof.

The Dis owner also skips Fast requests when its rooted RC queue is empty. Queue
occupancy is read only during the active exclusive heap interval. A nonempty
queue and shutdown drain keep the original MORE/fairness/retirement protocol.
Cycles requests remain independent; an empty Fast skip does not suppress a
needed cycle pass. NATIVERC still requires zero pending owners/direct bytes.

Real interpreter/JIT cyclic-heap gates exercise atomic and locked routes,
reclamation, saturation, large-block fallback and managed-error joins. For whole
process timing, from the installed SDK run
`python source/threading/benchmark_gc_poll.py ./disvm-editor ./threaded-owner.dis`:
this alternates five runs per route, checks identical application instruction
counts and reports every sample and medians. Times include process startup and
GC joins; they are observations, not opcode/UI latency or hardware guarantees.

The shared heap is still stopped during collection. Complete concurrent graph
barriers, atomic shared-heap mutation, parallel spawn and blocking Sys release/
reentry remain unfinished; the stop-request flag does not implement any of them.


The explicit DVM_WORLD_NO_ATOMICS build control exercises the conservative
fallback in the same native fixture. GCC native fast/reference, forced fallback,
five repeated Fast/Cycles rendezvous runs and strict UBSan native fixtures pass.
These are separate from the managed application gates. Executed target is Linux
x64; MSVC/ARM64/RV64 source branches are not new device execution evidence.

## Runtime0.44.0: audited native-call release/reentry

Workspace v0.1736 adds actual heap leave/reentry for Sys.sleep through
runtime_gc_native_leave/enter. This is an audited scalar callback, not a blanket
release/acquire implementation. The caller captures its scalar frame/result,
saves the current complete native-call REG in Prog.R and publishes roots through
native_world_leave before blocking. The root visitor can scan the inactive saved
stack. After OS wait the same mutator enters again before touching managed memory.
Calls with invisible unregistered managed C-stack owners are not admitted.

The first adapter is sleep_sys.c. It uses the shared thread port, checks native32
period range, returns zero for nonpositive periods and -1 for out-of-range values.
It contains no managed allocation, buffer access or throwing callback between
leave and enter. GC-protected regions retain ownership. Foreign/nested or broken
pairing is process-fatal before an unsafe managed return. The mutator context is
immutable through worker joins. Native call timing is independent of Flash frames.

Owner-private native release/reentry/progress counters are reported after joins.
Cycle progress is read only inside active world intervals before/after the wait,
not through racy reads while the collector writes. The managed fixture preserves
a cyclic graph/string/arrays and nested call frames; worker runs require actual
GC progress while released and six matching reentries. Console/UI each execute12
successes and 2 managed-limit error retirements, JIT/interpreter, inline/atomic/
locked and quanta1/2000. Existing cyclic-heap and real SVG editing gates also run.

Sys.sleep blocks this Dis owner. UI/GC can continue; other Limbo processes are
still cooperatively scheduled by that owner. Arbitrary Sys read/write callbacks,
opaque native module C roots, cancellation and exception paths require their own
buffer/root audits before release can be generalized. Concurrent graph marking
and parallel shared-heap spawn remain closed.

## Runtime 0.45.0: bounded native input and immutable output

Workspace v0.1737 adds audited heap release to Sys.read/readn/write. Typed FD/
byte-array admission lives in io_buffer.c; input and output algorithms are in
read_sys.c/write_sys.c. Main retains only coordination/test-control handling.
The generated native frame/result is captured before callbacks. Original FD and
array owners receive external roots before release; slice backing graphs remain
reachable. Native handles are captured on the active Dis owner, not queried
through managed metadata while the owner is inactive.

Each positive I/O call owns at most 65536 staging bytes. Reads target only native
storage while outside the heap, then revalidate and copy acknowledged bytes after
reentry. readn repeats fragments; EOF/error returns its already committed prefix
or-1 and leaves unfilled managed bytes untouched. Writes copy a bounded immutable
snapshot before leaving; only that snapshot reaches the port. Result publication
and all managed root retirement occur after reentry. Root-registration errors
unwind with exact native cleanup; port statuses are handled after ownership is
restored. NATIVEINPUT/NATIVEOUTPUT report actual current/peak staging, root pins,
fragments and copied/acknowledged bytes for explicit --stats.

Sys.write returns short acknowledged fragments; IDE file-service code already
loops correctly. No atomic-file/durability guarantee follows. Zero-byte requests
preserve legacy descriptor checks; zero-byte file writes keep their active-owner
port operation. Formatted output, generic C-stack owners, loader callbacks and
cancellation still require distinct release audits. The only Dis mutator still
blocks on OS I/O while UI/collectors can progress.

Real pipe/file gates check every byte of nonuniform binary input/output, sliced
arrays and untouched boundaries, partial EOF, wrong-direction native handles and
read-only/write-only port failures. Native writer tests size/check only their
private Linux pipe before an actual ready/backpressure/drain handshake. A second
real Dis process waits on a channel with array/cycle roots and checks them after
I/O. This tests saved-process roots without claiming extra Dis OS mutators.
Console/UI each run 12 successes and 2 managed-error retirements for each direction,
across inline/atomic/locked, interpreter/JIT and quanta 31/2000. Existing cyclic-heap,
native-wait, file-window/filesystem and graphical SVG editing gates also execute.

Both upstream snapshots import series 0068 bounded numeric source tuple results.
The scalar wire profile/reader stays unchanged. New 200-source images execute in
the existing x64 consumer and pass the existing independent reader; old 167 source
products retain exact ordinary Dis/Dis64 bytes across independently built compiler
versions. Tuple shape/producer/binding removal controls remain separate evidence
from whole VM execution. Concurrent marking and parallel spawn remain open.


## Runtime 0.46.0: formatted output and source value copies

Workspace v0.1738 extends audited heap release to Sys.print/fprint in the separate
formatted_sys.c module. xprint completes under active heap ownership; only its
bounded native C buffer reaches OS I/O while inactive. Captured FD handles remain
externally rooted. The published native call frame retains all managed format/
variadic arguments. Reentry precedes managed results and FD root retirement.
NATIVEFORMATTED records zero remaining pins and actual released port attempts.
The existing message bound and access/short-write semantics remain unchanged.

The new gc-native-format example/test checks exact UTF-8 stdout/file bytes with
a genuinely restricted OS pipe, variadic arguments, saved caller/cyclic roots and
a second channel-blocked process. Console/UI each run 12 successes and 2 managed
instruction-limit retirements across interpreter/JIT, inline/atomic/locked and
quanta 31/2000. Existing native wait now counts its final print as a seventh
release; previous six sleep releases remain. GC/join/TLS checks still pass.

Both Inferno snapshots import series0069 at a1d8cd9. Bounded pointer-free copies
snapshot before writing; flat numeric tuple arguments/results stay source-level
specialization of the unchanged scalar wire profile. Local x64 replay passes
241 sources / 482 existing C/JIT VM processes, 12 semantic controls and 24 refusals;
40 memmove/overlap comparisons and wrong-order control; tuple removal controls;
byte identity for old 200 Dis/Dis64 products. No parallel managed mutator, generic
release/acquire, loader/cancellation or concurrent marker is admitted by this.


## Runtime 0.47.0: exclusive heap turns with FIFO admission

The production GC-worker world uses dvm_world_create_serial. A bound owner's
enter reserves its FIFO turn; leave publishes immutable roots and permits the
next owner. Queued owners remain inactive and visible to stopped-root scans.
Collector stops/resume generations take priority over admission. Tickets/counters
are native unsigned values under the world mutex; no per-ticket allocation or
new per-instruction mutex is introduced. Foreign/nested entry consumes no ticket.
No thread cancellation is allowed while queued. The existing create/create_mode
parallel-admission API remains unchanged; its caller must synchronize mutation.

NATIVEGC adds serial_heap=1, admissions, contentions and waiters=0. Admission
counters are sampled before final owner leave/detach. dvm_world_admission exposes
coherent diagnostics, not a reservation. Existing audited native callbacks keep
their root/buffer contracts; this gate does not discover invisible C-stack roots.

The native_serial fixture uses two extended-up OS owners, one shared plain
operand and actual retained addw. Deterministic initial contention proves FIFO;
one owner then keeps its published roots unchanged while another makes at least
16384 arithmetic updates. Both stress 10000 reentries each. A collecting context
performs 201 exact two-root scans. Exact arithmetic, at-most-one active mutation,
root/TLS retirement and an active admission-removal control pass in atomic,
locked, no-atomics fallback and strict UBSan builds. The focused native TSan
run also completes without a data-race report; executable-name lookup warns and
limits symbolization. This is not TSan coverage of the whole managed VM. Reference two-active-mutator
and Fast/Cycles fixtures continue to test their original, separate contract.

Only one managed Dis executor is currently attached to the production scheduler.
Before attaching another, current-process assumptions, per-owner native-call
state, runnable/process lists, caches, channels and RC operations must be moved
under consistent ownership. Parallel managed spawn and concurrent graph marking
remain open. See ../tests/README.native-serial.md for commands and limitations.


## Runtime 0.47.1: prepared native frames remain owned during terminal unwind

Inferno 0070 (93f1738) repairs terminal destroystack ownership for prepared FRAME
arguments before CALL publishes FP, pending gaps below argument helpers and
unentered extensions. Allocated suffixes use captured SP/TS; validated pending
frames join ordinary mapped cleanup. Parent/type/module cursors are captured
before callbacks. No allocation, REG layout or shared wire change is introduced;
coherent-span refusal retains legacy diagnostic chain-only destruction. This is
terminal cleanup, not proof of recovery into arbitrary language handlers.

The owning trusted-local-call fixture and controls are imported verbatim and
registered as runtime CTests. The selected DisVM POSIX adapter supplies headers;
no sibling Inferno checkout or external emulator is needed. Local x64 strict
UBSan passes 448 C/RX cases each in WORD32/WORD64 and all four leaked-owner
removal controls. Previous stack-unwind/lifecycle and RET/MCALL matrices pass in
both models; 16 whole-runtime GC/FIFO/I/O/file-service gates pass after import.
This evidence stays separate from production parallel managed scheduling.

Inferno reviewed the serialized admission direction in INFERNO-0070. Root/REG
publication must precede release and reentry precede managed access; channels,
run queues, RC and loader/module caches remain under exclusion until audited.
Trusted local native calls require eight-byte result addresses even when WORD
is four bytes. They do not justify widening ordinary LEA or admitting serialized
pointers. Typed result/address-slot metadata requires shared agreement and
verification before any new file-call profile. Existing Dis64 source inlining
and 43-op np0 admission remain unchanged.


## Runtime 0.48.0: native-call state belongs to each TLS owner

NativeContext embeds opaque DVM_HeapOwner metadata; there is no new TLS key and
no per-thread managed heap copy. RuntimeGC retains shared service/root/cycle
state, but native phase, before-epoch and release/reentry/progress counters are
owner-private. Audited callbacks use the same owner attach/enter/leave/suspend/
resume helpers as the multi-owner fixture. A shared epoch is read only after
successful reentry. One pending progress sample blocks further transitions until
consumed; duplicate/foreign/out-of-phase calls refuse without hidden admission.

Last-active-owner sealing closes registration under the same world mutex that
joins roots. Peers prevent sealing. It is not a raw-pointer lifetime reference:
hosts keep the world/service alive through all attach attempts and join every
extra owner before controller stop. Internal runtime attach/detach requires empty
process slots and an initialized/live externally published service. Registration
alone does not create a managed Prog, make arbitrary Sys entry safe or migrate
scheduler ownership. Invisible managed C-stack roots still need explicit leases.

NATIVEGC owner_state=tls identifies the production route; native-call counters
belong to the controller. Extra owners must copy their own statistics before
detach, then aggregate only under an appropriate host join/ownership protocol.
They do not mutate a shared native_inactive/native_before flag. Context unbind
refuses attached metadata as well as outstanding world hooks.

The native_owner fixture checks two overlapping native leases, independent epoch
samples, FIFO and 201 two-root scans with real retained addw. Active removals
expose admission, epoch-capture and registration-seal defects. Atomic/locked,
no-atomics fallback, strict UBSan and focused TSan pass; symbolization warnings
remain, and this is not whole-VM TSan evidence. The same public C tutorial is
linked into the fixture and checks negative operation status and epoch wrap.
Nineteen runtime gates pass. See ../examples/native-owner/README.md for full
API contracts, source commands and installed SDK usage. The managed scheduler
still has one executor; queues/current-process/cache/channel migration is open.


## Runtime 0.48.1: prepared-frame caught recovery preserves full REG

Inferno 0071 at 7fe99c3 repairs caught emu/os exception recovery, complementing
0070 terminal cleanup. Validated suffixes release pending frames top-down at
every caller/extension; lower owners stay visible. Full resumed REG survives
argument/old-slot cleanup, module retirement and exception-owner transfers even
when actual destructor callbacks borrow all registers. No allocation, REG layout
or shared file-call admission changes. Caller-owned alignment padding in the
existing exception-frame fixture no longer represents an untyped pending gap.

Independent owning and selected-source strict-UBSan gates pass 5376 trusted
C/RX caught-call cases and 16 semantic owner/REG removal controls. Both ports and
WORD models resume to IEXIT with exact owners/registers/guards and zero remaining
fixture allocations. The actual exception/GC gate and 526000-case exception-frame
matrix per port/model pass. All 22 runtime/host gates pass after import. Three
new runtime CTests retain the selected-source handler matrix and controls.

Native-call metadata lives outside saved REG. The native-owner fixture explicitly
borrows ALL of its active REG, reads unchanged private phase/counters and restores
registers before any safepoint; this passes default and strict UBSan routes. That
is distinct from the trusted handler test and from whole-VM TSan evidence.
Parallel managed spawn, concurrent marking and arbitrary native cancellation
remain open. Snapshot manifests now pin 7fe99c3; the shared contract records the
jointly agreed FUTURE eight-byte result-address/byte32 rule, without enabling
LEA/CALL in the current 43-op file profile or assigning feature IDs.


## Runtime 0.49.0: runnable withdrawal before native release

The separate runqueue module keeps native continuations out of runnable dispatch:
complete REG publication → withdraw/Prelease with original owner → world leave;
then world reentry/epoch sample → same-owner prepend → managed result publication.
The separate process/root list is unchanged. Normal queue operations are O(1),
with metadata outside the retained Prog prefix/REG and no per-call allocation.
Foreign/duplicate restoration, ordinary append of leased Progs and premature
retirement refuse. Nonzero native64holds retains the existing synchronous-call
restriction; the generic TLS lease is not permission to bypass it.

The opt-in native-queue-probe starts a real extra runtime owner during actual
native suspension. It checks rooted/non-runnable/saved-frame state under its
heap turn and joins before the caller resumes. Alternating observers exercise
ACTIVE and IDLE service detach. High-level detach now retries an IDLE collector
refusal without attempting a second leave. Native owner/service lifetimes still
require external publication and joins. Baseline execution does not enable the
probe or create per-native-call observer threads.

An independent 20000-operation oracle and four active ownership controls pass
with real native identities, strict UBSan and focused TSan. Production console/
UI diagnostic runs cover eight normal/two managed errors per host and match all
withdrawal/reentry/GC/root/join counters. The managed dispatcher remains single-
executor; error/budget/current-process, callback wake/kill and queue/channel/cache
migration remain prerequisites for another. See RUNQUEUE.md for exact scope.


## Runtime 0.49.1: recovered roots precede fresh exception allocation

Inferno 0072 at 829d69f publishes the complete recovered REG into Prog.R before
cleanup/message construction may scan roots. rootset consumes that saved image;
physical unwind previously left stale bounds/freed extensions visible until final
PC selection. Both ports retain the final publication too. No allocation, register
layout, file profile or concurrent graph barrier is introduced by the repair.

Owning and selected strict-UBSan replay pass 21504 trusted fresh-message cases
and 16 semantic publication/map controls. Actual constructors, emergency failure
transfer, UTF decoding, rootset and recursive markheap execute; the caller-owned
two-object graph has no external-root substitute. The closure does not sweep or
run a concurrent collector. Real exception/GC epochs and 526000 frame cases per
port/model are separate passing evidence. All 28 configured runtime gates pass.

Host turn/error completion now clears up->prog along with the saved-process
publication boundary, preventing stale typed current identity between turns.
It remains present while the active error handler needs it; complete recovered
saved roots remain authoritative during borrowed-REG cleanup. Production queue/
I/O/GC/UI gates are rechecked after this bookkeeping change. Another managed
worker still requires per-owner error/budget dispatch and wake/kill migration.
Both snapshot manifests pin 829d69f; no wire/ID or default file admission changes.


## Source typing bursts (0.49.2)

Queued SDL text now crosses printable-key transitions, including digits,
punctuation and Unicode-layout keycodes, instead of only Latin A-Z. Committed
SDL_TEXTINPUT remains authoritative; no text is synthesized from key symbols.
Modifiers, commands, navigation, pointers and foreign windows retain FIFO
barriers; the256-event budget remains. Standalone text repaint now uses the same
allocation-threshold GC step as MDI, avoiding an unconditional full collection
per repaint. The Dis owner still owns edits; no optimistic document replica or
additional managed executor is introduced.


## Execution/error guards (0.50.0)

The extended native up now owns `DVM_NativeExecution`: the idle-turn count and
instruction-before-fetch quota flag. They are scalar native metadata AFTER REG,
never copied with Prog.R or traced as roots. `dvm_native_execution()` only borrows
the current bound typed owner; it cannot inspect another thread's context.
Each executor also uses its own up.defenv error/system-error buffers. The runner
has no process-global error-buffer fallback. The instruction-boundary flag is
cleared at each new process turn and retained through its error guard.

The total instruction/native counters and process/queue/heap remain shared and
must still be accessed under the exclusive heap turn. Only the idle-turn policy
is owner-local; the explicit total instruction quota is not multiplied per owner.
This is preparation for multiple executors, not a worker-pool switch.

The real OS-thread fixture runs4000 retained addw bodies, with distinct error
buffers and contradictory boundary flags. Whole REG overwrite/restore leaves
guards unchanged. After both workers join, their unbound contexts transfer to a
third OS thread and retain the owner state. An active shared-storage removal
control must fail a semantic predicate. Focused UBSan/TSan checks do not establish
race freedom of the whole VM. Wake/kill/native continuation routing and actual
parallel managed scheduling remain separate work.


## Managed-turn claims (0.51.0)

See [RUNQUEUE](RUNQUEUE.md#managed-turn-ownership-0510). Managed dispatch claims
now cover the complete turn, including native waits, independently of runnable
membership and native continuation leases. Success/error/block/retirement paths
finish on the original owner; a later ordinary wake permits reassignment to a
different native context. Production probes check balanced claims/finishes.
The heap is still exclusive and the production managed worker pool remains off.


## Two managed OS executors (0.52.0)

`--gc-worker --managed-workers 2` starts one additional real OS executor beside
the controller. Both call the same production dispatch function and can execute
C/JIT turns of the same Prog after an ordinary rotation. The default is1. This
mode requires the worker collectors; the graphical host also requires threaded
UI. It does not enable concurrent managed heap writers.

The shared NativeWorld FIFO gate protects queue, heap, roots, modules and JIT
caches. Before yielding an empty scheduler interval, both process slots and the
context turn token must be empty and `up.env` restored to native default. No
managed pointer is borrowed through this interval. Startup rendezvous and helper
join happen AFTER leaving the gate. Helper publishes its final native counters,
detaches and destroys its TLS on its own thread before collectors stop. Stop is
cooperative; a blocked native OS operation may delay join.

Prog embeds its own bounded error buffers and supported FP control/status. A
claim binds these logical values; every completion/error path saves them and
restores the OS owner's host FP/default environment before rotation/free.
`Sys_werrstr` captures the generated frame and writes a full native WORD result.
Native C exception jump stacks remain OS-local. The input-progress generation is
shared under the heap gate; each owner observes it before applying its idle
budget. The instruction limit remains global, not multiplied by executor count.

An audited native wait retains dispatch affinity to its original context, saves
REG, withdraws the Prog from the runnable queue and roots it through reentry.
Meanwhile the other executor can execute DIFFERENT runnable bytecode. Ordinary
FILE-backed operations keep heap exclusion in this mode: FVM_PortFile cursors
and errors require external serialization. Only audited standard I/O/sleep
release. General C callbacks and native64holds remain under their existing
restrictions. UI helper RPC is admitted only from its ACTIVE claimed turn in
the same world; copied requests share the existing serialized UI cell.

[Runnable examples and complete admission driver](../examples/managed-workers/README.md)
cover both executors, migrations, exact reference instruction counts, two native
writers under real pipe backpressure, shared-file contents, helper UI requests,
errors and quota teardown. Menu entry19 exposes the installed launcher. Counters
`NATIVEPOOL`, `NATIVEDISPATCH` and `NATIVEQUEUE` provide post-join evidence; they
do not claim simultaneous managed execution. Native Linux x64 is tested locally.


## Bounded executor pool (0.53.0)

Originally `--managed-workers 1..8` selected the TOTAL number of OS executors, including
the controller. Above1 requires `--gc-worker` and threaded UI in graphical
builds. Default is still1. That release's installed entry19 launcher defaulted to2;
`DVM_MANAGED_WORKERS=8` chooses eight without duplicate runtime options.

Each helper owns a stable native slot through join, including context identity,
bytecode-turn count and final owner counters. No managed roots live in these
slots. All helper identities used for UI authorization are inspected only under
ACTIVE heap exclusion. Stop sets one native predicate, releases the controller's
empty heap turn, joins EVERY successfully started helper, then reacquires heap
ownership before stopping collectors. Partial creation failure follows the same
join path; it never destroys shared storage while a helper can reference it.

The controller rendezvous waits outside the heap until every requested helper
has attached, or a failure is latched. Rootless scheduler handoffs are counted
separately from native-call releases/reentries: the latter can legitimately be
zero for a compute-only helper. Per-helper `NATIVEPOOL_WORKER` statistics are
read after join, not concurrently sampled.

The capacity gate verifies4/8 executors, positive bytecode turns for every
helper, exact instruction totals versus one executor, per-Prog errors/rounding,
UI RPC in the graphical host, global quota/error retirement, and eight real
blocked writers. Its private OS pipe is held full until inflight8; only then are
800024 payload bytes drained/counted. Peak bounded staging is524288 bytes.
The eight-writer fixture uses quantum2000 to avoid dominating this native-wait
check with array-initialization scheduler handoffs; existing quantum1/31/2000
compute checks remain. Heap mutation stays exclusive: this pool increases native
wait overlap, not the speed of a CPU-bound serial heap workload.


## Native per-file FIFO (0.54.0)

Ordinary Sys.read/readn/write/fprint/seek now release heap ownership in audited
calls. Each file slot owns a native FIFO gate allocated BEFORE destructive
open. The mapped FD is rooted through operation reservation, native wait and
reentry. Reserve while ACTIVE, save/release the native lease, wait for the file
ticket OUTSIDE the heap, perform native I/O, finish the file ticket BEFORE
reentering the heap. Neither the gate mutex nor heap gate is held through the
other gate's blocking admission. The native mutex protects only short ticket
updates and is not held during file I/O. Standard streams use no file gate.

Gate storage contains native counters/context identity only. Completion requires
the admitted ticket and exact current native context. Duplicate/foreign finish
refuses without advancing. Retirement refuses while any ticket is reserved or
busy; a runtime close in that state is a fatal ownership violation, never a
use-after-free. Supported FD aliases retain the same mapped owner; arbitrary
mutation/fabrication of runtime descriptor internals is not a new capability.

All runtime accesses to a file cursor/metadata share this gate, including private
loader reads/size and fstat. Metadata and private loader operations still execute
synchronously under their existing heap boundary, and may wait for an earlier
file operation. Open/create/close and directory callbacks are not made resumable
by this change. Zero-write validation stays active. Protected native64holds or
gchalt regions retain their established no-release rule. No forced cancellation
or abandoned ticket is allowed. Normal rooted calls always finish their ticket
before publishing managed errors or releasing their mapped FD.

`NATIVEFILES` reports created/closed/live gates, completed operations, contended
admissions, pending reservations and maximum per-file backlog after worker joins.
The legacy `serialized_files` pool counter now counts FIFO-admitted file calls.
[Examples and native-barrier test](../examples/managed-workers/README.md#ordinary-files-outside-the-heap-runtime0540)
verify unrelated managed progress while one native fwrite is deliberately held
and a second owner waits on the SAME file gate. This controlled test is distinct
from the existing real stdout-pipe backpressure test. Native Linux x64 is tested;
focused FIFO TSan does not establish whole-VM race freedom.


## Metadata and directory batches (0.55.0)

Sys.stat copies the validated UTF-8 path into owned native storage BEFORE native
release. Only the snapshot and copied FVM_FileInfo reach the OS; VM Dir/string
construction happens after reentry. A different actor may convert other paths
while this request is suspended. Native staging is freed on every returned
status. Allocation failure publishes no metadata record.

Sys.fstat pins the mapped FD, captures the ordinary file or directory handle,
reserves its gate, then releases the native lease. Completion finishes the gate
BEFORE heap reentry, consumes the lease and unpins before VM publication. Open
directory slots now allocate the same FIFO gate as files. Sys.dirread uses this
protocol for one bounded native batch, then constructs mapped records/strings
under the heap gate. C batch storage stays native and is released after success,
EOF or protected VM publication failure. Abort on publication failure shares
the directory gate and retains the active error-cleanup boundary.

Concurrent readers serialize native cursor operations, not VM publication order
or a whole-directory transaction. Already completed batches cannot be revoked by
a later failure; abort prevents future reads under the native port contract.
No callback may hold a descriptor gate through managed reentry. Open/create/
close, private loader calls and protected native64holds/gchalt remain synchronous.
One managed heap still has exclusive mutation; native OS waits are cooperative.

`NATIVEMETADATA` counts path/descriptor attempts, native releases, FD pins, PATH
staging/current peak and directory batch attempts. Native batch allocation is
owned by the port and not included in path-byte counters. The Linux/glibc barrier
test pauses real stat64/fstat64/readdir64, then uses stdin tokens to prove another
actor runs DURING each wait. It also checks65 shared-cursor entries exactly once
across32-entry batches, C/JIT4/8 owners and post-join gate/root retirement. Native
Linux x64 is the local execution target; no whole-VM race/platform claim follows.
See [installed examples](../examples/managed-workers/README.md#metadata-and-directory-waits-runtime0550).


## Elastic executors and parked task lifetime (0.56.0)

The current policy supersedes the historical eight-executor pool: use
`--gc-worker --managed-workers auto` for demand growth or a positive fixed count.
Menu19 defaults to automatic mode; the ordinary runtime default remains1.
There is no eight-executor or128-task policy limit. Stable linked native slots
are allocated separately; completed helpers destroy TLS, publish completion and
are joined outside the heap by any rootless executor before their slots are freed.
After64 consecutive empty scheduler intervals, extra helpers retire to the
controller-plus-one-helper baseline. Native completion markers avoid growing the
pool when an executor is already returning to the heap. Runnable publication,
retirement and stop wake generation-based native conditions.

Automatic mode preserves channel-waiting tasks after module init returns.
Logical channel listeners do not consume individual OS threads. Optional
`--managed-workers-limit N` (N>=2) limits live executors. Replacement resource
failure is fatal, not recoverable cancellation. Fixed/default entry-lifetime
policy is unchanged. Heap mutation remains serialized; audited native overlap
and executor migration do not establish parallel managed compute.

[Release and validation scope](../../docs/releases/v0.1752.md) |
[Remaining work, Russian](MULTITHREADING-ROADMAP.ru.md).


## Native file/directory open and create (0.57.0)

Sys.open/create now snapshot the path and construct unpublished native handles
outside the heap lease. An allocated descriptor gate reserves the table slot
before release; other opens skip reserved slots even though neither handle is
published yet. A marked retained mapped FD survives the wait with fd=-1; the
completed handle and descriptor publish only after reentry. Port failure returns
nil and frees the gate/snapshot. Original string2c scratch is never reused after
release. Loader opens and GC-driven descriptor close remain synchronous.

NATIVEOPEN records completed attempts/releases and staging/peak snapshot bytes.
The seven-phase metadata/open barrier test requires competing UTF-8 creation and
managed progress before releasing real fopen64/opendir/open64, plus error cleanup.
See [release and scope](../../docs/releases/v0.1753.md).


## Native finalizer retirement (0.58.0)

With --gc-worker, FD finalizers detach handles only after native pins/FIFO users
are gone and enqueue a retirement job allocated BEFORE file creation. A separate
native closer owns only port handles; it never admits the heap or calls VM code.
The gate/job/handles leave their descriptor slot before queue publication, so
slot reuse is independent of a slow physical close. Shutdown stops producers
and collectors, then drains/joins the closer. Reference/startup close stays inline.

NATIVECLOSE shows transfer/completion/backlog/join evidence. A permanently blocked
OS close still blocks final join; cancellation and native-resource backpressure
remain separate work. See [release](../../docs/releases/v0.1754.md).


## All-executor deferred RC (0.59.0)

Deferred last-reference admission now checks ACTIVE/pending-progress/world identity,
not equality with the original OS controller. Helpers can enqueue and request Fast
work. Native/idle/detached/foreign/collector callers refuse before payload access;
collector recursion cannot requeue. Queue limits and inline fallback are unchanged.
NATIVERC adds helper transfer/fallback counters. Managed heap mutation and collector
passes remain exclusive. [Shared-heap proposal, Russian](SHARED-HEAP-CONCURRENCY.ru.md)
describes object locks/atomic RC with initially STW cycles, not an activated mode.


## Typed native count conversion (0.60.0)

The new disvm/refcount.h API uses checked native unsigned/signed counters and
acquire/release RMWs, with a separately tested port-mutex fallback. Selected
host module/inspection/table ownership updates and RC eligibility use it.
Unconverted retained C/JIT/Type/marking paths still require exclusive world
admission. Counter operations are not object lifetime acquisition from a raw
pointer, GC roots, marking, or a parallel queue. Heap layout is unchanged.
[Exact conversion boundary](REFCOUNT-CONVERSION.md).

Runtime0.90: [preallocated root groups](ROOT-BATCH.md), a prerequisite for mapped-copy retirement; general managed execution remains serialized.

Runtime0.91: [mapped MOVMP transaction](SHARED-MAPPED.md), opt-in guarded publication and rooted retirement; general managed execution remains serialized.

Runtime0.92: [rooted HEADMP](SHARED-HEAD.md), guarded source-list capture and mapped projection. Other shared writers/GC epochs still require NativeWorld.

Measured costs and pending investigations: [PERFORMANCE-COSTS.md](PERFORMANCE-COSTS.md).

Runtime0.93: [rooted HEADP/TAIL](SHARED-LIST-POINTER.md), opt-in source lifetime plus guarded pointer projection. General managed execution remains serialized.

Runtime0.94: [HEADB/W/L/F/M contract and evidence](SHARED-LIST-BYTES.md).
Source containers stay rooted through bit-preserving guarded projection and cleanup;
NativeWorld remains required for general managed execution.

Runtime0.95: [integer conversion transaction](SHARED-INTEGER.md) holds the exact
source/result byte union through capture, conversion and publication. NativeWorld
is still required; native OS executors do not yet imply parallel managed compute.

Runtime0.96: [binary integer transaction](SHARED-ARITHMETIC.md) guards both
inputs and result together. Native mixed two-/three-span tests use real joined OS
threads; general managed computation remains serialized by NativeWorld.

Runtime0.97: [compact small ranges](SMALL-RANGES.md) use six boundary stripes
in the same domain as wide/cell operations. This reduces metadata work without
removing owner/root/NativeWorld requirements.

Runtime0.98: [MUL/DIV/MOD](PRODUCT-DIVISION.md) keep both inputs/result under one
claim; DIV/MOD return semantic errors after unlock, preserving zero precedence
over invalid outer sentinels. General managed admission remains serialized.
