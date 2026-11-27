# Live variables through Styx VFS

[Русский](STYX_RPC_LIVE_VARIABLES.ru.md) · [Bridge](STYX_RPC.md) · [C SDK](../../lib/styxrpc/LIVE_VALUES.md) · [Runnable example](../../lib/styxrpc/examples/disvm/LIVE_STAGE.md)

Use a live binding to inspect or change a variable published by the other process. Each operation addresses one scalar or selected element. The peer owns the underlying storage. Use [typed RPC values](STYX_RPC_VALUE_TYPES.md) for a copy of an entire array or string, or for ref copy-back at the end of one call.

## Select a projection

| Publication | Get and set | Additional methods |
| --- | --- | --- |
| [Int32 cell](../../disVM/runtime/VFS-INT-VARIABLE.md) in `.var` | `get()`, `set(number)` | `close()` |
| [Int32 array](../../disVM/runtime/VFS-INT-ARRAY.md) | `get(index)`, `set(index, number)` | `length()`, `close()` |
| [Scalar big](../../disVM/runtime/VFS-BIG-VARIABLE.md) | `get()`, `set(numberOrDecimal)` | `close()` |
| [Big vector](../../disVM/runtime/VFS-BIG-VECTOR.md) | `get(index)`, `set(index, numberOrDecimal)`. Negative indices count from the end. | `length()`, `close()` |
| [Hybrid table](../../disVM/runtime/VFS-HYBRID-TABLE.md) | Same methods with exact numeric keys and relative negative indices. Use [bindHybrid](STYX_RPC_HYBRID_BIND.md) for separate string keys and structural operations. | `length()` for the dense portion, `close()` |
| [String cell](../../disVM/runtime/VFS-STRING-VARIABLE.md) | `get()`, `set(stringOrNull)`. Limit 1,024 UTF-8 bytes per value. | `close()` |
| [Record of published fields](../../disVM/runtime/VFS-VARIABLE-RECORD.md) | Open a field, then use that field's methods. | `field(name)`, `length()`, `close()` |

Ordinary live bindings are local to one mounted session. `binding[0]` and assignment to `binding.name` are ordinary script property operations. Use `get`, `set` or `field` to reach remote data. The record is an explicit list of capabilities supplied by the owner. Use a separate field for a nested record. A writable Int32 module reference can also be [forwarded to an authorized process](STYX_RPC_INT_VARIABLE_BIND.md) through the specialized Int32 reference API.

## Retain a field and update an element

Given the record in the [paired example](../../lib/styxrpc/examples/disvm/LIVE_STAGE.md), capture a writable record, extract its Int32 array and discard the parent fid. The example publishes positions 0 and 2 as aliases of one native Int32. A successful write to position 0 is observable both at position 2 and from Limbo.

```actionscript
var record = vfs.liveVariable("/live/.var", true); // Upper bound on write access.
var numbers = record.field("numbers");           // Independent field fid.
var parentClosed = record.close();
var before = numbers.get(2);                      // -17 in the runnable fixture.
var updated = numbers.set(0, 42);                 // A single acknowledged write.
var aliased = numbers.get(2);                     // 42 on the same native cell.
var released = numbers.close();
```

The bound path and Qid path are verified on every read, write, length query and field capture. A content update keeps the same Qid path. Republishing a different node under the same name makes the old proxy stale. The transfer always uses the originally captured fid. If the owner replaces a path during a call, the call may complete against the original node or fail. To access the replacement, explicitly open it again. Release any stale aliases and close the session after its bindings.

## Use on a movie or a worker

The movie session and a mounted `asinc` session have the same methods. The operations are synchronous on their context owner. In a worker, stop interrupts a pending walk, metadata fetch or value exchange where the transport can flush it. An unacknowledged final write might have taken effect. Inspect the current binding or the owner's state before any replay. A cancelled selector is retired independently. Use one owner and serialize operations on the session's indexed projections.

## API reference

[`vfs.liveVariable`](STYX_RPC_LIVE_VARIABLE.md) · [`get`](STYX_RPC_LIVE_GET.md) · [`set`](STYX_RPC_LIVE_SET.md) · [`length`](STYX_RPC_LIVE_LENGTH.md) · [`field`](STYX_RPC_LIVE_FIELD.md) · [`close`](STYX_RPC_LIVE_CLOSE.md)

[`vfs.packSparse`](STYX_RPC_PACK_SPARSE.md) and [`vfs.unpackSparse`](STYX_RPC_UNPACK_SPARSE.md) are local helpers for a typed by-value copy.
