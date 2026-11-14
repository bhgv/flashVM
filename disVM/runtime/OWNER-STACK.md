# Owner-produced call-chain snapshots

Copyright (c) 2026 bhgv

[Русский](OWNER-STACK.ru.md) · [Runnable example](../examples/resident-stack/README.md)

Each completed owner boundary now publishes a bounded copied Dis call chain with
its existing location. It contains at most eight entries, top first. Entry zero
is the current original-Dis next PC. Lower entries are saved caller return PCs,
not call-site/source-line estimates. Original frame addresses never leave capture.

## Ownership and consistency

The permanent OS owner borrows its own live TLS module, FP and rooted frame chain
while holding the active managed claim. Normal completion and installed-handler
boundaries use the same producer. Starting a new claimed turn clears the working
record. No foreign thread walks a stack and idle mailbox service reads native
copies only. No global stop/exclusive, forced pthread suspension, Prog migration
or reload from a saved register image is introduced.

A local call inherits the current module; a saved nonnil `mr` crosses a module
call. Headers use memcpy without assuming frame alignment. Traversal visits at
most eight headers, detects repeated visited frames and refuses invalid sentinel/
overflow geometry or unavailable return coordinates. These are checks on trusted
live owner metadata, not validation of arbitrary native pointers or a sandbox for
corrupted native stack memory. Stack-extent headers/Type maps are not exported.

Every entry has the same saturating owner boundary sequence as the top location.
A completed ticket is immutable even as the guest returns, drops a module or
terminates. Native records retain no frame/module pointers, managed references or
GC roots. Reaping may remove the ticket between separate projections; callers must
discard partial reads on any non-Ready status. A ticket does not pin the guest.

This has bounded cost, not zero overhead: copying happens at each completed
boundary; mailbox records and resident publications are larger fixed native
buffers. There are still eight credits per target and no per-query native
allocation. Full memory accounting and global/client quotas remain separate work.

## Processctl projections

All three methods peek without consuming the original requester's credit:

| Method | Result |
| --- | --- |
| residentstack(id, ticket) | status, boundary sequence, count, stack flags |
| residentframe(id, ticket, frame) | status, sequence, next/return PC, instruction extent, name bytes, path bytes, location flags |
| residentframebyte(id, ticket, frame, part, index) | status, unsigned UTF-8 byte; part 0=name, 1=path |

Summary count is 0..8. StackComplete=1 means the root was reached; StackTruncated=2
means more callers exist beyond eight entries; StackUnavailable=4 means the valid
prefix ends at an unavailable position or invalid/repeated trusted link. Exactly
one classification is published. Sequence/flags/count all zero means no boundary
has been captured yet. Ready describes the ticket, not completeness of the stack.

Invalid index/part/copied geometry returns DiagInvalid; non-Ready payloads are
zero. Foreign requesters get DiagDenied. Existing scalar residentreply consumes
Ready/Closed; residentforget releases the query. Peeking keeps even Closed credits
until ordinary consume/forget/reaping. Pending inside a native callback is not an
instruction to interrupt that callback. Names/paths retain the existing bounded
UTF-8/LocationValid/truncation semantics and are display labels, not unique IDs.

## Limbo and acceptance

`module/residentstack.m` / `debug/residentstack.b` provide
`inspect(id, attempts) -> (status, ref Stack)`. Attempts1..1000 bound polling, not
total latency. The library copies every frame/label into caller-owned data and
forgets its ticket on normal/Pending/managed-error exits, rethrowing managed
errors. Partial results are discarded; locals permit shared-library concurrent
calls. If cleanup cannot execute, existing requester/target retirement reclaims
bounded native credits. Injected managed errors are not actual allocator-OOM tests.

The native fixture covers imported Unicode modules, local recursion, shallow
complete chains, depth80 across stack growth, bounded truncation, frozen frames
after return/drop, later caller-only capture, original-requester denial, index
bounds, scalar consumption and provider final release. Public example and injected
reader-error/Pending cleanup execute in C/JIT with fast/locked polling; the imported
fixture additionally executes quantum1. Native units check cycles, local/imported
links, unaligned headers, malformed return and bounded depth. Two semantic omissions
remove publication or corrupt copied caller labels; exact DETECTED output and full
normal native/GC drain are required. Crashes/timeouts/sanitizers never count.

Typed variable contents, source symbols, unique module catalog, pause/resume/step
and external debugger capabilities/transport remain unimplemented here. This is a
bounded Dis call-chain snapshot, not a native C stack or a full debugger.

Final affected selection: 11/11 ordinary and 11/11 ThreadSanitizer groups pass,
including control/registry/mailbox/location, the new stack workload, cancellation,
retirement and table ownership/errors. Successful production provider extent growth
is explicitly probed. Mandatory architecture list now contains59 workloads (list
verified); this selected run does not claim the whole new architecture matrix.
