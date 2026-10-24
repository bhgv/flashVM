# Inferno GC, native fast/cycle workers and exact roots

[Русский](GC.ru.md) · [Shared host](README.md) ·
[Worker API](../include/disvm/gc_workers.h) · [Implementation](gc_workers.c) ·
[Native-world contract](../include/disvm/native_world.h)

The selected backend remains Inferno's retained
[`destroy`](../upstream/libinterp/heap.c) and
[`rungc`](../upstream/libinterp/gc.c): reference-count reclamation plus incremental
colour-based tracing for objects that reference counting cannot reclaim.
No Lua heap, allocator or object representation is imported in this checkpoint.
The objective is low retained memory with bounded background work, not a claimed
benchmark win over Lua or an unimplemented moving/compacting collector.

## Recognizable up groups

The [extended up](native_registers.c) now retains the actual member names:

```c
/* Proc remains the compatible prefix; the active register set stays named R. */
typedef struct DVM_ExtendedUp {
    Proc Proc; /* Error state, current process and OS/FPU environment. */
    REG R;     /* Active registers owned by this native thread. */
} DVM_ExtendedUp;
```

Saved `Prog.R` remains separate. The prefix layout is preserved; no members of
REG were flattened, widened or silently converted between Dis words and native
pointers. New GC configuration/state is also grouped explicitly as `Fast` and
`Cycles`, each with its own named budget, callback and pass counters.

## Two real native workers

[`dvm_gc_workers_create`](../include/disvm/gc_workers.h) starts two OS threads
through FlashVM's platform thread port and binds distinct collector contexts.
`Fast` drains deferred reference-count work. `Cycles` advances bounded tracing
passes. Requests are coalesced flags, not an unrooted queue of native heap pointers.
Idle workers sleep on FlashVM condition variables; there is no periodic whole-heap
polling while idle. The platform sources cover Linux, Windows, macOS and Android;
executed acceptance here is Linux AMD64.

Each pass acquires a heap-phase lease, stops registered mutators at instruction
boundaries, publishes their roots, calls the selected backend, then resumes them.
Fast and cycle passes never mutate the same heap concurrently. Mutator instructions
run in parallel outside these short coordination phases, not under one execution
mutex. When both lanes have work, the turn alternates so one cannot repeatedly
starve the other. This is cooperative stop-the-world coordination per pass, not
a fully concurrent collector with atomic write barriers.

The backend returns DONE, MORE or FAILED. MORE requests another bounded pass;
optional `pause_us` paces it outside heap/world locks (maximum one second).
A practical starting policy is Fast budget 1 with no pause, Cycles budget 1 with
1000 microseconds between MORE passes. These are tunable work units, not a hard
pause-time promise: one real `destroy` can recursively release a large graph, and
one `rungc` uses Inferno's own adaptive scan quota. Neither pacing nor scheduling
changes the retained algorithms.

Failures are latched and further requests fail. A successful pass must return
normally; callback exceptions/longjmp cannot cross worker/world cleanup. Shutdown
discards queued requests, finishes an in-progress pass and joins both threads.
The backend keeps ownership of unprocessed references. `wait_idle`/destroy cannot
be called from a collector callback; active mutators must leave their mutator
interval before waiting for a collection that needs their own roots.

## Strong references and cycles

Type/frame maps, not pointer-shaped bit patterns, define managed references.
Root reachability includes module data, native/interpreter roots, typed stack
frames containing locals/parameters, saved/active register state, pending calls
and explicit host-owned roots. A deferred RC queue must itself retain/root its
pending objects until the fast worker consumes them. Existing `ptradd`/`ptrdel`
provide one retained root-registry mechanism; updating that registry also needs
proper mutator/collector synchronization.

Alignment and membership in a live allocation can be useful diagnostic checks,
but do not establish a strong edge. An aligned integer equal to an object address
must not keep a dead cycle alive. Conversely, valid mapped pointer bytes can be
copied from an unaligned record: existing pointer-map helpers use byte-safe loads.
Arrays/slices and native handles require their explicit metadata/ownership rules;
arbitrary interior addresses cannot be guessed into managed roots.

Trace from the known roots through known strong slots. Objects left unreachable
are candidates for reclamation even if their internal RC counts are nonzero.
Tracing once from roots is preferable to independently searching backwards from
every suspected pointer. The retained Inferno colour/mark protocol supplies this
reachability machinery; this host does not replace it with a heuristic scanner.

Low live memory and physical compaction are separate concerns. Fragmentation,
arena release and relocation belong to the allocator. Moving objects additionally
requires correct updates to every root/edge, interior reference, native binding
and JIT address. This series implements no automatic relocation or compaction.

## Evidence and remaining integration

[`inferno_gc.py`](../tests/inferno_gc.py) executes the actual retained RC/GC bodies,
with a deterministic **test pool**, not the production arena allocator. It checks:

- cascading acyclic reference-count release on the Fast OS thread;
- unreachable cycles reclaimed by real rungc on the separate Cycles OS thread;
- cycles reachable from module globals or a mapped stack parameter remain alive;
- dropping those strong edges permits reclamation;
- an unmapped scalar equal to an object's address does not retain a cycle;
- queued deferred-release ownership is pinned through the real pointer-root registry.

The control fixture checks startup/callback failure, fair MORE scheduling and
self-join prevention. A separate combined fixture runs two real addw mutators
while the Fast/Cycles threads perform 100 root-stop/scan/resume phases. That
combined fixture verifies coordination, not heap reclamation; the allocator
fixture verifies the retained algorithms. They are distinct acceptance scopes.

The scheduler library is real, but the complete Dis host still needs its
production allocator and root adapter, synchronized mutator RC/heap operations,
blocking-operation hooks, module-table ownership and safe finalizers. Existing
`gclock`/`gcunlock` are gchalt counters, not native interthread mutexes. No blanket
claim of shared-heap thread safety follows from those macros. Resource/UI
finalizers must obey owning-thread/channel rules rather than entering FlashVM
or drawing from a collector thread. The library does not yet redirect every
interpreter `destroy` call into a production deferred queue or run a complete IDE.
