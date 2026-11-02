# Runnable processes and native continuations

`runqueue.c` owns one heap's runnable FIFO. `main.c` retains scheduler composition;
`runtime_gc.c` coordinates roots/heap turns. `queue_probe.c` is an opt-in observer.
No retained upstream Prog layout or file calling ABI is changed. The host factory
allocates a native wrapper with the full Prog prefix plus queue membership and
continuation-owner metadata. Freeing the prefix releases that same allocation.
The existing 128-process quota includes native waiters.

Normal append/pop/withdraw/restore are O(1), using native membership metadata.
Only explicit diagnostics walk the bounded list. All queue reads/writes require
the heap turn (or single-owner startup/joined teardown); these are not arbitrary
lock-free operations. `currun()` now uses this OS owner's typed `up->prog`, rather
than treating a shared runnable head as current-process identity.

An audited native call follows this order:

1. Validate active TLS ownership and existing synchronous-call restrictions.
2. Copy complete REG to the current Prog, preserving caller/frame/type roots.
3. Remove the current runnable head, mark Prelease and record its native owner.
4. Leave the heap with process/root-list identity unchanged.
5. Perform only audited native work; this Prog cannot be normally appended or
   dispatched. Its C stack/continuation stays on the original OS thread.
6. Reenter the heap, sample the collector epoch, then restore this SAME owner at
   the runnable head before publishing managed results/continuing interpreter code.

Ordinary wake/append and foreign/duplicate restore refuse outstanding leases.
The terminal process-retirement path refuses a live continuation before removing
its roots or destroying frames. The guard does not invent cancellation/wakeup
semantics: foreign C channel waiters remain unsupported by this host. Module/
frame/C-held owners stay subject to the existing audit and root-lease rules.

A nonzero retained `native64holds` is a synchronous Native64Caller lease. It
refuses suspension and retains heap ownership; it is not silently promoted into
a resumable TLS native lease. General `release/acquire` callbacks remain separate
from these explicitly audited Sys paths. Whole REG borrowing does not copy the
native queue-owner metadata.

## Real production observer

Build normally, then run a matching native-wait module:

```sh
./disvm --gc-worker --native-queue-probe --stats native-wait.dis
```

From an installed SDK, compile the documented example first if needed:

```sh
./limbo-native -g -I include/limbo -o native-wait.dis examples/gc-native-wait/main.b
./disvm --gc-worker --native-queue-probe --stats native-wait.dis
python3 source/threading/test_queue_probe.py ./disvm ./native-wait.dis
```

The diagnostic option requires worker GC and is off by default. It starts/joins
one extra real OS owner at every released native call, so this is instrumentation,
not a benchmark configuration. That owner uses actual runtime attach/detach and
observes the real leased Prog under its own heap turn: absent from runnable FIFO,
Prelease, original continuation owner retained, still linked among actual GC
process roots with saved frame/module state. It never executes that Prog.

Normal native-wait runs report seven matching controller releases, withdrawals,
restorations, observer joins and root observations (six sleeps and final print).
`NATIVEQUEUE` must have inflight=0. `QUEUEPROBE` reports matched observed/joined,
failures=0 and explicit IDLE detach entries. Alternating observers retire directly
from ACTIVE or deliberately enter IDLE first; the latter checks retryable service
detach without depending on whether a collector wins a race.

If GC starts between extra-owner leave and detach, an IDLE refusal is retried
without a second leave. The world/service remains live through every attempt and
join. This corrects the former high-level wrapper's ACTIVE-only retry behavior;
it does not reference-count arbitrary borrowed service pointers.

## Independent algorithm checks

From the source checkout:

```sh
python3 disVM/runtime/test_runqueue.py
CFLAGS='-O1 -g -fsanitize=undefined -fno-sanitize-recover=all' \
  python3 disVM/runtime/test_runqueue.py
CFLAGS='-O1 -g -fsanitize=thread' python3 disVM/runtime/test_runqueue.py
```

The real queue is checked against an independent array oracle for 20000 operations,
including quota/full/empty, FIFO, blocked state, repeated native round trips,
separate root links and bounded cycle diagnostics. Another actual OS context
claims the same current Prog but must fail restoration/ordinary admission.
Four active removals expose synchronous-hold promotion, foreign restoration,
duplicate runnable admission and missing withdrawal. Only their exact semantic
markers count; compiler faults/timeouts are failures. Positive default/UBSan/TSan
runs pass; TSan warns about executable-name lookup, limiting symbolization. This
focused test uses real Prog declarations but is not a complete managed VM.

Production console/UI tests run eight normal and two managed-limit error cases
per host, covering interpreter/JIT, atomic/locked and quanta1/2000. They also check
collector progress, native pins, observer/context joins and UI/Dis TLS separation.
No latency threshold is used. The actual managed scheduler still has one executor;
per-owner errors/budgets, dispatch, queues/channels/caches and kill/wakeup paths
need further migration before enabling another. Current 43-op Dis64 admission is
unchanged.


## Managed-turn ownership (0.51.0)

The host-only Prog wrapper now carries a `dispatch_owner` independently of its
native-continuation `native_owner`. `claim` assigns only the runnable FIFO head
to an idle bound context, publishing its current Prog; duplicate, nested and
foreign claims refuse. The retained Prog/REG layouts are unchanged. A claim is
not itself a heap/root lease: all operations still require heap exclusion and
the host must keep the context bound/alive until `finish` completes.

Retained execution keeps the claimed head in the FIFO because current-head
scheduler primitives still consume it. Only its original dispatcher may pop it.
Native suspension removes it from the FIFO while retaining its dispatch claim;
paired same-owner reentry restores it before the C continuation resumes. Another
OS owner can hold a turn claim on a DIFFERENT runnable Prog during that wait.
It cannot claim or finish the suspended Prog. No saved REG contains these owners.

`finish` retires exactly one complete turn after stable register publication and
native reentry, before rotation/free. It also accepts a blocked Prog no longer
in the FIFO and an error path whose TLS process root was already cleared. This
permission is based on the native claim, not arbitrary current-process identity.
After finish and ordinary wake, a different native context can claim that SAME
Prog for its next turn. There is no permanent thread affinity for managed Progs.

The runner uses this protocol for success, caught error, fatal error and normal
module retirement. `NATIVEDISPATCH` reports claims/finishes/active separately from
native wait leases; production probes require equality and zero active at exit.
One managed executor remains enabled. A production pool must still preserve
context lifetime, safe wake/kill routing and queue/channel/module/cache exclusion;
this protocol does not make arbitrary C stacks or concurrent heap writes safe.

The independent FIFO fixture retains20000 oracle operations and eight active
semantic controls. Real extra OS contexts challenge a claimed head, forge a TLS
process pointer, claim another Prog while the original native continuation is
withdrawn, and reassign a finished/woken Prog. These are real queue/context
operations, not managed bytecode execution on a pool. The production extra-owner
probe checks claims under the actual shared heap gate during interpreter/JIT I/O.


The opaque NativeContext ALSO retains an independent native turn token. This is
not the mutable GC process slot: clearing that slot during error cleanup must
neither permit another turn on the same owner nor permit unbind/destroy. Claim
acquires that token; finish releases the exact identity after native reentry.
A failed finish restores a previously cleared process slot if token validation
fails, without releasing shared ownership. Foreign finish is checked at both
the Prog and context-token layers. One semantic control intentionally weakens
BOTH layers; weakening only one must still refuse. Additional controls remove
the nested-token and early-unbind guards independently. No token is serialized
or treated as a managed root; the scheduler retains the Prog in its root list.


## Two-executor dispatch (0.52.0)

The opt-in managed worker calls the same dispatch function as the controller.
Ready-head claim remains the sole admission; all queue/Prog fields are protected
by NativeWorld heap exclusion. `last_dispatcher` is a numeric diagnostic identity,
never a borrowed NativeContext pointer. It detects ordinary-turn migrations;
native continuations still require their original live dispatch/native owner.

The runtime wrapper now embeds logical Osenv and supported floating control/
status. These follow Prog; the context token, host FP save slots and C jump stack
stay OS-local. Restore the default environment before freeing a wrapper. Helper
yield/join refuses any nonempty process slot/token. See
[the lifecycle protocol](THREADING.md#two-managed-os-executors-0520).


## Bounded pool (0.53.0)

Up to eight native executors share the same claim/finish and continuation-affinity
protocol. Pool slots are native-only and stable through all joins; they never
replace root-list membership or retain a borrowed NativeContext after detach.
Per-slot identities are cleared while the helper still owns the heap. Every
started helper joins before slot/service reset, including partial-start failure.


## File waits (0.54.0)

An ordinary file native continuation uses the same Prog withdrawal, pinned
context and reentry protocol as standard I/O. Its native file FIFO ticket is
reserved while ACTIVE and finished before managed reentry. A waiting file owner
is never left in the runnable queue; a different owner can execute another Prog.
File gates do not replace managed FD roots or NativeWorld root membership.


## Metadata and directory waits (0.55.0)

Path metadata uses native UTF-8 snapshots. File/directory metadata and batch
reads retain explicit mapped FD roots and the same per-slot native FIFO. Every
queued native continuation stays off the ready FIFO until its original owner
reenters. Native batches are never managed objects; publish VM records after
reentry and never retain a descriptor ticket through heap admission. Protected
publication failure aborts the directory via a fresh serialized operation.
