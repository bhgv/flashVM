# Owner pump and retained UI input

[Русский](OWNER-PUMP.ru.md) · [Bridge](UI-BRIDGE.md) ·
[Pump ABI](include/disvm/ui_pump.h) · [Client ABI](include/disvm/ui_client.h)

The pump connects an actual FlashVM queue pair to nonblocking callbacks executed
by the service owner. It is an embedding component, not a new scheduler or a
standalone Dis executable. It borrows its bridge, callbacks and endpoint state.
A step visits both directions for a caller-bounded number of rounds. It does not
wait for a service, channel peer or UI capacity.

An ingress record consumed from the transport remains in the pump until the
service accepts it. An egress record consumed from a service remains until the
UI queue accepts it. Each direction retains at most one extra record. Busy ingress
does not prevent polling egress. Acceptance counters count actual transfers and
saturate; they do not claim exactly-once execution or deduplication.

A terminal callback/transport error stops future routing and preserves pending
records for inspection. `dvm_ui_pump_pending` copies them without consuming them.
Stop closes the transport and wakes waiters. Destroy frees only inactive routing
state; the host must first stop/join owners and explicitly recover or abandon
pending values. Callbacks must not reenter, block, throw or longjmp across the pump.

The supplied Dis callbacks use the real `csendalt`/`irecv` paths under the caller's
Dis ownership. `kprotect` catches Dis failures and active REG is restored on both
success and failure. Received byte-array references are decoded and released.
Channels must be registered as `chan of array of byte` and rooted by the host;
a pointer mover cannot prove their static Limbo element type. The production
adapter uses TLS registers. The extraction fixture compiles the same bodies with
its explicit global-REG fixture owner, not with a fake TLS acceptance claim.

`DVM_UIClient` adds a 64-record UI-owner FIFO ahead of the 32-record transport.
It validates and copies input/requests before local acceptance. Partial flushes
preserve the unsent head and ring order; close preserves undelivered records.
Use the new capture-only pointer/keyboard functions after ordinary FlashVM dispatch,
post the exact returned value, then flush on later UI ticks. Never recapture an
old transition from newer context state. Producers requiring mutual ordering must
all use this FIFO; bypassing it can overtake locally pending records.

These components introduce no alternate OS input stack, no cross-heap pointers
and no replacement for Limbo rendezvous/ALT. Automatic frontend hook registration,
module/process lifecycle, complete heap/GC ownership and the executable Dis host
remain integration work. Native tests cover retained ingress/egress, real Dis
movers, injected allocation failure, register restoration, exact FlashVM input,
ring wrap and shutdown. Fixture scheduler/pool boundaries remain explicit.
