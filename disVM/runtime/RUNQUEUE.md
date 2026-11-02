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
