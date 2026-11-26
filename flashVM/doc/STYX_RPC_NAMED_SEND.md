# Named channel send(value)

[Русский](STYX_RPC_NAMED_SEND.ru.md) · [vfs.chan](STYX_RPC_NAMED_CHANNEL.md) · [Code](../src/styxrpc/session_named_values.cpp)

`port.send(value)` waits until a reader on another participant accepts
the value and returns `undefined`. An ordinary variable supplies its current
value. Strings, numbers, Booleans, null and undefined use portable tags. Arrays,
tables and objects use the FlashVM graph codec, including supported shared
edges and cycles. One encoded frame may contain up to 65,536 bytes including
its type byte. Values without a graph encoding and oversized frames raise an
error before sending.

`port.send(ref counter)` sends a reference to a writable AS scalar variable.
On the first send, the source slot attaches to a
[libstyxrpc cell](../../lib/styxrpc/reference/named_cell.md). Ordinary reads
and assignments in AS then act on its current value. Further sends of
`ref counter` preserve its identity. The receiver can bind a fresh name,
assign a different scalar kind, and forward the same reference.

The supported live slots are plain writable variables and own properties
holding a number, string, Boolean, null or undefined. Slots with custom
accessors, watchers, or object values are outside this publication profile.
Ordinary `send(value)` supports serialized object graphs. Previously issued
Int32 references can still be forwarded on their issuing session.

```actionscript
var counter = 7;
var sent = port.send(ref counter);
var current = counter;
```
