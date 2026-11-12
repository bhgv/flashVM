# Shared ordinary module retirement

Ordinary hosted `Modlink` finalization can now run on shared Fast while other
residents remain admitted. Instance MP ownership moves to a rooted packet before
the last link reference is claimed. The existing synchronized Module registry
controls final metadata destruction. This extends the
[Fast finalization series](FAST-FINALIZATION.md); it does not remove the general
stopped tracer, every native finalizer or stopped shutdown.

## Supported identities and prerequisites

[module_retirement.c](gc/fast/module_retirement.c) admits the exact static
`Tmodlink` heap identity with a live owned Module and a valid MP value. Dynamic
code/data (`DYNMOD`), dynamic-library handles and native instance data are excluded.
No arbitrary heap Type's free callback is invoked. Ordinary bytecode and builtin
Module metadata use the existing native cleanup; JIT allocations remain owned by
their Module until its final release.

The link's queued strong owner and root keep its fields and Module alive through
its reader cutoff. Other execution frames or links must independently own their
module references. Registry lookup retains under the same short mutex that
last release uses to unlink; a pointer cannot escape between zero and removal.
Destructors, code unmapping and managed releases run outside that registry mutex.
The new path adds no mutator execution gate or global graph lock.

## MP handoff and failure

[module_data.inc](gc/fast/module_data.inc) runs only on the sole shared Fast
consumer while RC still accepts publication:

1. For `MP == H`, atomically claim the last link reference without allocating a
   child packet. There is no fabricated MP reference to consume.
2. Otherwise publish the existing MP owner in a rooted release packet before
   claiming the last link reference. The count itself is not incremented: the
   original owner moves from the link field into the packet.
3. On a successful claim, clear `link->MP` to `H` and commit the packet. Fast
   subsequently removes the link's root and finalizes its known Module owner.
4. If allocation/root publication fails while the packet queue is accepting,
   preserve link/MP/reference and request a shared retry. If a competing retain
   defeats the actual claim, clear/destroy the staged root batch and cancel the
   reservation before retrying. Never detach the source MP on refusal.

Fast's Module finalizer withdraws its private `link->m` field and calls ordinary
`unload` with shared admission and RC acceptance still live. A nonfinal Module
release preserves the remaining owner and registry entry. A final release unlinks
first, then `destroy(origmp)` publishes that existing MP owner through the shared
RC path before `freemod` releases types, code, names and other native metadata.
The packet's cloned MP and Module's original MP each retain their own descriptor
and field owners until their later physical Fast reclamation.

Closed RC acceptance cannot safely run recursive `unload` here. It preserves the
existing retained route rather than making an impossible shared publication retry
forever. Unsupported module identities also retain that route. This series does
not establish stop-free shutdown or dynamic native-module cleanup.

## Actual allocation and reader evidence

[test_module_retirement.py](test_module_retirement.py) links real allocator,
registry, loader, compiler, RC and collector objects with private observers.
A real Limbo app starts two permanent peer residents that alternate epochs while
at least one always remains admitted. No heap-quiescent collection can explain
successful target retirement.

A native observer receives the final ticket alias and reads real Module metadata
under an old reader epoch. Its next epoch starts after the ticket cutoff and
before RC publication. That later reader separately protects Module metadata and,
when present, the actual executable code mapping. Only after repeated real Fast
responses may it end. The observer never manufactures Fast progress.

Six positive profiles run C/JIT and both poll implementations:

| Profile | Required actual behavior |
| --- | --- |
| Final ordinary Module | Free link, cloned MP, original MP, shared string and native Module metadata before peers leave |
| Lookup-acquired owner | Link/clone retire while Module remains registered; removing the lookup owner later retires original MP and metadata |
| MP root publication refusal | Preserve source MP/count, then publish and physically retire on shared retry |
| Competing retain | A real temporary reference defeats last-count claim; rollback all staged roots and later succeed |
| Empty Module | Free the link and metadata with `MP == H`, no Type vector or invented child allocation |
| Compiled Module | Compile a genuine target JIT cache, preserve its readable code through both reader epochs, then observe successful real `munmap` before metadata free |

Arena generation keys distinguish each target allocation from later address
reuse. A target heap must have no remaining registry root at actual `poolfree`;
completion is counted only after the allocator returns. The independent Type
creator owner must return to one reference after both MPs and Module release
their owners. Metadata and executable-code identities are withdrawn before their
actual free/unmap, so later address reuse is not mistaken for the old allocation.
Unrelated native frees/unmaps bypass the observer mutex, avoiding inversion with
native root/allocator locks.

The compiled target uses a real one-instruction compiler input and real executable
mapping. Its emitted complete tail-entry actually runs and tail-jumps to a private
C dispatcher observer. That callback retires its own original Module/code mapping;
it then returns directly to the original C caller after successful munmap. This
checks that no return address remains inside disposed module code. The target's
ordinary IRET handler is replaced by the observer, so this is a tail-lifetime
check rather than complete execution of a target Dis program. The hosting Limbo
application itself runs in C and x86-64 JIT modes. Generated machine instructions
are not C-TSan instrumented.

Four private omission controls must fail with status 12 and an exact predicate:
disable shared module eligibility, omit MP transfer, bypass the leaf epoch, or
skip packet rollback after the real competing retain. Compiler failure, timeout,
unrelated abort or sanitizer warning never counts as detection. The relay/reader
barriers are separated into
[test_module_retirement_relay.inc](test_module_retirement_relay.inc); construction
and actual tail invocation live in
[test_module_retirement_case.inc](test_module_retirement_case.inc). Every changed
handwritten source stays below 500 lines.

## Remaining boundaries

This initial pass handles last ordinary Modlink owners and their native metadata.
The [module-instance cycle follow-up](MODULE-CYCLE-RETIREMENT.md) now adds ordinary
`Modlink -> MP` edges and graph finalization. Native Module/original-MP cycle
closure, arbitrary native finalizers, complete root publication and shutdown
without stopped ownership still require work. Module
construction/Loader mutation and every external code-cache borrower are not
comprehensively audited by these fixtures. The dynamic-module exclusion and
closed-admission routing remain explicit prerequisites, not complete shutdown or
native-plugin fault-injection coverage.

Validation is Linux x86-64. ARM64/RV64 runtime execution remains unverified. The
[current roadmap](MULTITHREADING-ROADMAP.ru.md) and prior investigations in
[CONCURRENT-GC.md](CONCURRENT-GC.md) remain the overall acceptance criteria.

## Validation

The ownership/root/module/resident selection passed 82/82 normal tests. Ten
additional native/idle cycle and graph GC tests also passed. The final role unit
and module acceptance, including all four semantic controls and actual emitted
tail execution, were rerun separately and passed.

Ten final full-runtime ThreadSanitizer repetitions completed all 24 positive
profiles: 240 application executions, 40 observed target executable mapping
unmaps and their successful returns to C after unmap. No sanitizer warnings
appeared. These are local observations of the exercised paths, not a claim of
complete race-freedom or complete concurrent collection. Earlier acceptance
investigations remain in the linked reports.

[Original-MP cycle retirement](ORIGINAL-MP-CYCLES.md) extends native ownership
claims to sole-owner Modules, including rollback and event-driven leaf wakeup.
