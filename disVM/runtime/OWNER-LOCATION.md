# Owner location captures

Copyright (c) 2026 bhgv

[Русский](OWNER-LOCATION.ru.md) · [Runnable example](../examples/resident-debug/README.md)

A resident now copies its current module labels and **next original Dis PC** while
its permanent owner still holds the active managed claim. A diagnostic ticket
keeps the copied data, without retaining the Module, Modlink, stack or any managed
pointer. Later execution, return from an imported module and final module release
do not modify an already completed reply.

This is a display location at a completed execution boundary. It is not a stack
trace, a global Module ID, native instruction pointer, source line, breakpoint or
permission to inspect memory. A next PC may equal the instruction count at an end
boundary. Resolving it to source requires matching module/debug metadata; copied
names and paths alone do not establish unique module-instance identity.

## Capture and lifetime

`diagnostics/location.c` reads only the active owner's rooted Modlink/Module and
its own live TLS PC. Normal `xec` completion and installed exception-handler
boundaries publish a native copy before releasing the claim. Starting a new
claimed turn clears the old working record. Idle diagnostic service copies the
last publication; it never reads another thread's registers or walks a heap.

The geometry check uses integer spans, refuses unrelated/misaligned/native-PC
coordinates, and accepts original Dis offsets from zero through `nprog` inclusive.
The current leaf-JIT path retains these original Dis coordinates. Legacy
native-PC `compiled` modules are explicitly unavailable. Module code extent,
name and path must be immutable while the caller holds their lifetime. This
function is not validation of arbitrary native pointers or mutable module data.

Module names have 64 bytes of native storage and paths 256, including NUL. The
copied text is at most 63/255 bytes. Truncation stops at a complete UTF-8 prefix;
invalid captured UTF-8 clears the whole location. No allocation, callback or
Module reference is acquired during capture. Work is bounded per completed
boundary; this instrumentation is not claimed to have zero overhead.

The owner-local boundary sequence saturates at signed-big max. It identifies
progress within that resident, not a Module. The immutable record is identified
by the original requester and exact target/ticket within the current VM service.
There is no cross-VM generation authentication or external debugger transport.

## APIs

Existing `residentrequest` creates the ticket. Existing scalar `residentreply`
remains compatible and consumes a completed reply. Two new projections **do not
consume** Pending, Ready or Closed credits:

| Processctl method | Result |
| --- | --- |
| residentlocation(id, ticket) | status, sequence, next PC, instruction count, name bytes, path bytes, flags |
| residentlocationbyte(id, ticket, part, index) | status, unsigned UTF-8 byte; part 0=name, 1=path |

Non-Ready output is zero. Invalid part/index and malformed copied geometry return
DiagInvalid. Foreign requesters receive DiagDenied and no bytes. LocationValid
means the geometry and copied labels are available; name/path truncation have
separate bits. A Ready reply with LocationValid clear has no available location,
although its scalar progress reply is still valid. Use residentreply or
residentforget to release the credit after peeking.

The target may be reaped between projections. Discard a partial read on any
non-Ready result. The ticket does not keep the target alive. A blocking native
callback can keep a request Pending until its owner reaches a servicing point;
these APIs do not interrupt that callback or pause the VM.

## Limbo reader

`disVM/debug/residentdebug.b`, declared by `module/residentdebug.m`, provides
`inspect(id, attempts) -> (status, ref Location)`. It submits, polls, copies both
labels into caller-owned strings and attempts to forget the ticket on normal,
timeout and exception exits. An injected managed error tests rethrow and complete
quota reuse. Pending timeout tests verify that ordinary native sleep continues.

Attempts must be 1..1000. This bounds polling iterations, not total scheduler or
native-call latency. If the VM cannot execute cleanup itself, requester retirement
and target destruction still reclaim the bounded native credits. A shared library
instance has no mutable globals; each caller owns its local capabilities and data.

## Verification and remaining work

The imported-module fixture checks exact Unicode labels, next-PC bounds, frozen
peek data after return/instance drop, later caller boundary sequence, requester
isolation and final provider release. The public example and managed-error/Pending
cleanup fixtures execute in C/JIT with both polling profiles. The imported caller
also runs with quantum=1. Native probes verify active owner/TLS, UTF-8 truncation,
end-PC, native-PC/misalignment refusal and invalid text. Two omissions suppress
publication or alter copied path; exact DETECTED diagnostics must follow normal
completion and full native/GC drain. Crashes/timeouts/sanitizers never count.

The owner-location workload is part of the mandatory architecture list (58
workloads at that stage). [Bounded call-chain capture](OWNER-STACK.md) is now available.
Typed-variable capture, stack depth beyond that prefix, unique module catalog, pause/resume,
source symbols and external capabilities remain separate work.

The final affected regression selection passes 10/10 groups in the ordinary build
and 10/10 under ThreadSanitizer, including mailbox/control/registry, cancellation,
retirement and table ownership. This is selected regression coverage, not a claim
that the full new architecture matrix has already completed.
