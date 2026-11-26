# Named channel send(value)

[Русский](STYX_RPC_NAMED_SEND.ru.md) · [vfs.chan](STYX_RPC_NAMED_CHANNEL.md) · [Code](../src/styxrpc/session_named_values.cpp)

`port.send(value)` waits until a reader on another participant accepts
the value and returns `undefined`. An ordinary variable supplies its current
value. Strings, numbers, Booleans, null and undefined use portable tags. Arrays,
tables and objects use the FlashVM graph codec, including supported shared
edges and cycles. One encoded frame may contain up to 65,536 bytes including
its type byte. Values without a graph encoding and oversized frames raise an
error before sending.

A value snapshot samples supported live entries in the whole graph. These
include named cell bindings, the Int32 variable adapter, and transparent AS
aliases that terminate in own data or one of those cells. Other getters are
never invoked during capture. A missing own target or an unreadable shared
cell fails the send before it enters the rendezvous. The standalone
[serialization API](../include/flashvm/serialization.h) keeps its original policy.

The snapshot preserves cycles and shared object identities. Aliases to one
named StyxRPC cell are sampled once per graph, including when they arrived
on separate leases. Distinct cells are sampled separately. The received
entries contain plain values, so writing one cannot update the original
cell. A native table keeps its numeric indices and string names distinct.
The same snapshot rule applies when a graph becomes the value of a cell.

`port.send(ref counter)` shares a plain writable AS variable or own property
through a [libstyxrpc cell](../../lib/styxrpc/reference/named_cell.md).
On the first send the source slot attaches to the cell. Subsequent reads and
assignments access its current value. A new binding after `recv()` can change
the value, including its type, and forward the same cell with `ref`.

Cells hold the same scalars and serializable object graphs as value messages.
A read of a graph slot materializes a local snapshot. To publish edits, assign
the edited snapshot back to the shared slot. For direct updates to one field
of a local object, share it with `port.send(ref model.count)` or pass a computed
key with `port.send(ref rows[index])`. The slot must already exist as a writable
own data property, Array element, or native numeric table entry. Direct reads and assignments such as
`rows[index] = 42` then act on the shared cell through ordinary AS access. Custom accessors,
watched properties and values the graph codec refuses cannot be published.
Previously issued Int32 references still forward on their issuing session.

```actionscript
var counter = 7;
var sent = port.send(ref counter);
var current = counter;
```
