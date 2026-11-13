# Cancellation operations retain their own identity

A PID identifies a child, not one `cancelwait` attempt. A parent can abandon
attempt A when its own wait is cancelled, then post B to the same live child
before A completes. Matching only the PID lets A's result complete B.
The old child request and parent reply were also reused embedded nodes:
reposting while an earlier operation was in flight violated exclusive node
ownership, and another reply could overwrite a queued result.

Each asynchronous attempt now receives an increasing 64-bit ID scoped to its
parent lifetime. Exhaustion is an explicit error before parking; IDs never wrap.
The consumer requires the expected child PID, reply kind and operation ID.
Retirement replies keep their separate, one-shot child-PID protocol.

One native allocation belongs to each asynchronous cancellation:

| Boundary | Owner and disposal |
| --- | --- |
| Before request publication | Caller; allocation/posting failure leaves no queued node. |
| Request queued or taken | Child mailbox/consumer; abandoning the parent wait does not revoke it. |
| Completion | Child reuses the same node as the reply; no shared reply slot. |
| Parent still waiting on that child | Reply is transferred; parent accepts the matching ID or discards a stale ID, then frees it. |
| Parent absent or no longer interested | Child frees the request. |
| Endpoint teardown | Both queues drain and free owned cancellation nodes. Embedded channel/retirement nodes are retained. |
| Stop after native reply staging | The resident frees its staged cancellation node before releasing its worker slot. |

The registry mutex protects parent lookup and wait metadata. It does not stop
other residents, replace their TLS registers, or hold a native test wait.
No global NativeWorld admission or fallback was added.

`test_cancel_generation.py` compiles real Limbo with three permanent native
owners. A table wait makes A's cancellation genuine. Its owner is delayed before
the actual table cancellation; the grandparent cancels the parent's wait, and
the parent posts B to the same child. A then returns `Cancelled`; B returns
`NotWaiting`. The observer withholds B until the real parent consumer has
processed A, so overwriting a shared reply cannot hide stale acceptance.
The fixture checks exact outcomes and all three unique cancellation allocations
are freed. It runs interpreter/JIT and both GC polls with strict no-stop evidence.

A private omission control removes only the operation-ID check. It must execute
the same interleaving and fail with `stale cancellation A completed new wait B`;
compiler failures, timeouts and sanitizer diagnostics do not count as detection.
The portable wide suite includes the positive probe on AMD64, ARM64 and RV64.
RV64 CI remains QEMU functional coverage, not hardware concurrency certification.

The older live-parent stale-reply race probe retains 4096 observations but gives
each call its own owned message. It no longer invokes a consuming completion
repeatedly with the same storage. Its TSan old-reader control remains enabled.
