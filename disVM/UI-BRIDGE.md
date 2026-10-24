# Dis ↔ FlashVM channel bridge

[Русский: полный контракт](UI-BRIDGE.ru.md) · [Example](examples/ui-channel/README.md)

`DVM_UIBridge` owns two real FlashVM channels, one per direction, bounded to
32 messages each. Creation selects buffered or actual rendezvous semantics.
The ordinary AS channel wrappers interoperate through FlashVM's primitive-string
fast path. There is no second queue, input stack or graphics implementation.

The native Dis adapter copies the envelope into an actual Tbyte array and offers
it using production `csendalt`/`movp`, with implicit buffer allocation disabled.
Its destination must be a host-registered `chan of array of byte`; a pointer mover
alone cannot prove the Limbo static type. The reverse adapter validates a rooted
received Dis byte array and copies it into the UI queue. Both adapters run under
the Dis execution/heap owner. Dis allocation errors require the host's normal
error/kprotect boundary. Internal Dis channels, ALT and process scheduling remain
unchanged. This local envelope is not a 9P/Styx wire protocol.

The wire record is `fvd1:` followed by six eight-digit lowercase hexadecimal words
(kind, id, code, flags, first, second), then at most 1024 canonical UTF-8 bytes.
The header is 53 bytes. kind is Request/Reply/Pointer/Key/Text/Focus (1–6).
Requests/replies require nonzero uint32 correlation IDs; events use zero.
The last two words are signed int32; others are uint32 bit patterns. Pointer
coordinates use 20 twips per pixel, truncated toward zero. Embedded NUL and
supplementary Unicode are supported; lone surrogates and noncanonical UTF-8 are
rejected consistently. Neither pointers nor native struct layouts cross heaps.

Input capture reads already-dispatched FlashVM pointer/keyboard state on its
owner thread. It does not install global hooks or redispatch events. Each capture
returns the exact copied event before attempting a nonblocking send; on pressure,
retain it and retry the send rather than recapturing changed state. The host must
preserve event order and handle every failure. Text/IME and focus classes exist,
but automatic frontend routing is still pending. AS alone mutates MovieClip/UI
objects on the UI owner; that thread must never block waiting for a service.

Success means transport acceptance, not operation completion. FIFO is per direction;
correlation does not imply deduplication or exactly-once execution. Close wakes
waiters and allows buffered messages to drain. Join/quiesce users before releasing
wrappers and destroying the bridge. Invalid packets are consumed/released and
leave the output record unchanged.

Linux AMD64/CMake 3.10.2 acceptance: 42 tests passed. Changed bridge tests were
repeated after Unicode/input refinements. They cover actual FlashVM AS wrappers,
input dispatch/state, bounded FIFO, rendezvous, close and production Dis array/
channel movers. The Dis fixture still supplies scheduler, pool and error boundaries;
it does not execute a complete Limbo application. Both supplied Limbo modules
compile with the source-built bootstrap, and our Coco/R AS2 frontend accepts the
AS helper. Optional Node checks validate its codec algorithm, not FlashVM execution
of the entire helper. Node is not a product/build prerequisite.

Remaining host work includes module loading/registration, scheduler/lifecycle,
GC ownership, exact typed-channel registration and an owner pump retaining pending
messages. Full `uiecho`/`idecore` execution in our DisVM, frontend input routing and
the actual AS editor widgets remain unverified/unfinished.
