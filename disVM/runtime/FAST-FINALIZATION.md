# Shared Fast finalization, 2026-11-12

The managed-ticket path now proves physical reclamation of a real final string
while peer residents remain admitted, rather than proving only its handoff to RC.
The main runtime still uses mixed collectors; this is not completion of the
[no-STW plan](MULTITHREADING-ROADMAP.ru.md).

## Publication and optional failure

`dvm_runtime_rc_defer` previously forced every shared-service final owner into
the mandatory FIFO. Eligible final owners now enter the existing epoch-gated Fast
leaf queue from both ACTIVE and SHARED admission. The leaf owns the existing strong
reference and publishes its root before the ticket root/guard lease is removed.
No arbitrary destructor callback becomes eligible through this change.

If optional leaf metadata/root allocation fails, mandatory publication still
preserves ownership in the rooted FIFO. A bounded shared Fast drain can rescue
eligible final owners from that FIFO into leaves, publishing the replacement root
before withdrawing the old root. It can also drop a nonfinal non-candidate owner.
Nonfinal cycle candidates retain their prior discovery opportunity; unsupported
callbacks remain queued for the existing retained algorithm. Every visited FIFO
head consumes a budget unit, including allocation refusal/unsupported geometry,
and the initial ready count limits rotation of a busy head within one slice.

This does not make mandatory allocation OOM recoverable. The existing mandatory
publication contract still terminates on such failure instead of losing a strong
owner or invoking an unsafe inline destructor. Monitoring preserves its existing
retained path. Closed queues, unknown finalizers, graph refusal and shutdown still
need further work before the retained path can be removed.

## Separate grace periods

A ticket's reader cutoff and the subsequently published leaf's reader cutoff are
distinct. Readers starting after the ticket cutoff may still observe the object
before RC publication. Shared Fast waits for the leaf cutoff before decrementing
the final reference or touching final payload storage. A blocked leaf now returns
an explicit retry rather than reporting no shared work and requesting a stopped
fallback. This is a paced service retry, not a wait on all mutators.

Repeated full-runtime TSan runs also identified a race during resident retirement:
peers read the global controller fallback `initial_guest` while the controller
cleared it. The fallback is now native thread-local, initialized and cleared only
on its owning controller. Peer residents continue to use their original dedicated
slot and never borrow that controller pointer. Controller pending-work checks
remain on the controller thread; no foreign lifetime claim is implied by TLS.

Two shared-path increments of `helper_enqueued` now use the same atomic update as
other producers. Joined diagnostics continue to require balanced accepted/released
ownership, independently of how many internal queue transitions occurred.
`NATIVEMANDATORYRC shared=N` counts successful mandatory FIFO rescues, not visits.

## Real heap evidence

[test_managed_retirement.py](test_managed_retirement.py) exercises the unchanged
production allocator, RC and Fast code through native linker observers. Its two
resident peers alternate epochs so at least one always stays admitted. A native
observer drops the last exported native alias under an old reader epoch. It then
starts another reader after the ticket cutoff and before leaf publication, keeping
the actual string payload readable until Fast has requested several shared retries. Native world API observers also reject any
attempt at stopped admission during this later reader, even if admission would
fail because the peers are active.

The target `poolfree` must happen exactly once, after both required cutoffs, with
zero remaining target roots and no world stop while peers are admitted. Completion
is published only after the actual allocator returns. The independent externally
rooted string must retain exactly one managed owner and its one external root.
Checks also require empty ticket/guard state and successful resident continuation.
Normal execution, RC refusal and optional leaf-root allocation refusal run with
C/JIT and both poll implementations. The optional-root refusal must exercise the
mandatory queue and still physically free the object before releasing peers.

Private controls disable ticket processing, skip the ticket cutoff, omit its root
removal, disable mandatory rescue, disable physical shared Fast or bypass the leaf
cutoff, or omit its retry and request stopped admission. Each must fail with its precise semantic message and status 12. Compilation
errors, timeouts and sanitizer reports do not count as detection.

## Validation limits

The focused suite initially passed 75/76: receiver cancellation once reported
`cancelled receiving victim did not retire`. Eight repeats of that four-profile
driver with the modified binary and eight with the original RC implementation
relinked into the same build all passed. That comparison does not establish a
cause or resolve the observed failure; it remains an acceptance investigation.
The broader known failures in [CONCURRENT-GC.md](CONCURRENT-GC.md) also remain.
No complete race-freedom, concurrent collection of every cycle, ARM64 or RV64
execution is claimed. Generated JIT instructions are not C-TSan instrumented.

The RC implementation is split into [private source modules](gc/common/RC-SOURCE.md)
by responsibility. This keeps each changed source below 500 lines and preserves
one translation unit, without changing the runtime ABI.

After the TLS correction, ten full-runtime TSan repetitions completed the twelve
normal/RC-refusal/optional-root-OOM × C/JIT × polling profiles (120 applications).
The final stop-attempt audit then passed three more instrumented repetitions
(36 applications), without sanitizer warnings. A normal 30-test GC/lifecycle
selection passed, including dedicated/idle/native cycle GC and the large record
cycle workload. These cycle tests still exercise a mixed collector runtime.

The final 76-test ownership/root/resident/native-boundary selection also passed
after the TLS correction and the stop-attempt controls. The earlier single
receiver-cancellation failure is retained above as an unresolved observation,
not erased by the successful repeats.
