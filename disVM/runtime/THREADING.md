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
`--gc-inline` remains the default reference. A single collector OS thread binds
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
