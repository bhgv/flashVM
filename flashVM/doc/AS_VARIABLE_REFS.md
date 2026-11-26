# ActionScript variable references

[Русский](AS_VARIABLE_REFS.ru.md) · [AS2 compiler](../../compiler/as2/view/README.md) · [Executable check](../tests/styxrpc/peer-ref.as)

In the AS2 view profile, `ref counter` captures an existing root movie
variable. `ref model.count` captures the evaluated owner and its own property
name. Expressions such as `ref (counter + 1)` have no storage and are rejected.

```actionscript
var counter = 41;
var ch = channel.create("ref", 2);
var queued = ch.send(ref counter);
var alias = ch.recv();
alias = alias + 1;
var snapshot = alias;
var forwarded = ch.send(ref alias);
```

Receiving a reference into a new name installs an alias. A read fetches the
source's current value. A write uses the source variable's ordinary assignment
rules. `ref alias` retrieves the captured reference, including after forwarding.
`ch.send(alias)` reads the current value and queues that value as a separate
message.

`channel.create("ref", capacity)` creates a same-context FIFO of 1 through
65536 elements. `send` accepts a scalar or reference and returns false when
the FIFO is full. `recv()` returns the oldest message or undefined while the
queue is empty. `close()` discards the queue. The FIFO and the aliases keep
their source owners reachable through GC.

The reference FIFO stays within one context. Live Int32 references from
another process use [wire descriptors](../../lib/styxrpc/reference/int_variable_refs.md).
A read followed by a write uses two operations.

[VFS named channels](STYX_RPC_NAMED_SEND.md) also publish fresh scalar
slots on the StyxRPC session.
