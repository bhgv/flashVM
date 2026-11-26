# ActionScript variable references

[Русский](AS_VARIABLE_REFS.ru.md) · [AS2 compiler](../../compiler/as2/view/README.md) · [Executable check](../tests/styxrpc/peer-ref.as)

In the AS2 view profile, `ref counter` captures an existing root movie
variable. `ref model.count` captures the evaluated owner and its own property
name. `ref rows[index]` also captures the evaluated owner and a computed key.
The key is evaluated once at capture. The same captured slot is used if the
variable holding the index later changes. Expressions such as `ref (counter + 1)`
have no storage and are rejected.

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

The binding needs an unused name. Receiving another reference into an
occupied slot fails with [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47). If a frame handler also
waits for a peer, bind each alias once and repeat the readiness check
on later frames. [Paired player acceptance](STYX_RPC_PLAYER.md#run-acceptance)

`channel.create("ref", capacity)` creates a same-context FIFO of 1 through
65536 elements. `send` accepts a scalar or reference and returns false when
the FIFO is full. `recv()` returns the oldest message or undefined while the
queue is empty. `close()` discards the queue. The FIFO and the aliases keep
their source owners reachable through GC.

The reference FIFO stays within one context. Live Int32 references from
another process use [wire descriptors](../../lib/styxrpc/reference/int_variable_refs.md).
A read followed by a write uses two operations.

[VFS named channels](STYX_RPC_NAMED_SEND.md) also publish fresh writable
slots on the StyxRPC session. A slot may hold a scalar or a serializable
graph. A graph read returns a snapshot and whole-slot assignment publishes it.
Existing Array elements and Object properties are supported. In an Array,
`1` and `"1"` identify the same element. In a native table, these are
two independent slots. Capture an existing own numeric entry with
`ref table[index]`. The supported AS numeric indices are integers from
0 through 4,294,967,295. The C table API preserves its existing integer key type.

`table[aliasIndex] = port.recv()` can bind a reference at an unused numeric
index of a native table. Reads, writes and subsequent reference capture
use the same cell. Removing a numeric entry with `delete table[index]` drops
that binding. A peer with its own live cell lease can still access the cell.

An ordinary `port.send(table)` reads supported live slots into a detached
graph. The receiver sees their values as ordinary stored entries. The
[named-channel snapshot rules](STYX_RPC_NAMED_SEND.md) apply to nested tables,
arrays, and objects as well. Use `ref` to forward a live slot itself.

[Paired AS](../../lib/styxrpc/tests/flashvm/named_pair.as) ·
[Paired Limbo](../../lib/styxrpc/tests/flashvm/named_pair.b) ·
[C table API](../include/flashvm/table.h).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
<!-- source-links:end -->
