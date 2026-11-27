# Exact live VFS variables

[Русский](LIVE_VALUES.ru.md) · [C SDK](README.md) · [Paired example](examples/disvm/LIVE_STAGE.md) · [FlashVM reference](https://github.com/bhgv/flashVM/blob/master/flashVM/doc/STYX_RPC_LIVE_VARIABLES.md)

A hosted C application can work with published scalars, indexed collections and named fields on its existing Styx client. Link `StyxRpc::launch` and include `<styxrpc/live_client.h>`. Use an ordinary attached session. The variable API shares the connection with discovery and calls. No caller PID is needed for an admitted VFS path.

## Projection contract for providers

On each published node, return a stable Qid path and file kind while the same object is available. Allocate a distinct Qid path for a different publication at the same name. Walks from an already captured fid must stay relative to that incarnation. Each root or child must support bare walk and stat. Keep .type and metadata gated by file permissions.

| Node | Type and I/O |
| --- | --- |
| Int32 file in `.var` or field of a typed record | Regular readable file with length 4. Value is four bytes, signed big endian. |
| Fixed Int32 array | `.type` is `array of int
`. `.len` is an unsigned decimal count followed by newline. Write a four-byte unsigned index to `.idx`, then access `.val` as an Int32 file. |
| Big vector | `.type` is `vector of big
`. The length uses the same format. Select with an eight-byte signed big-endian index. Read or write an eight-byte signed value at `.val`. |
| Numeric space of a hybrid table | `.type` is `table of big
`. `.len` reports dense length. The selector is one zero byte followed by an eight-byte signed big-endian key. A negative index selects relative to dense length. `.val` uses the vector's value encoding. |
| Named record | `.type` is `record
`. `.len` counts named fields. Each field is a child published in its own type. |
| String or scalar big | `.type` is `string
` or `big
`. `.val` holds one complete S-record as specified below. |

A string `.val` uses `STYXRPC_VALUE_STRING` (tag 7) with at most 1,024 bytes of UTF-8, or `STYXRPC_VALUE_NIL` (tag 9) with empty payload. A scalar big uses `STYXRPC_VALUE_BIG` (tag 2) with its ordinary signed integer encoding. A complete write at offset zero replaces the value. A read fid serves one snapshot, even when several reads are necessary. These files use the normal Styx transport and UniBus codec. A provider must admit the whole value in one write at its negotiated message size. [Shared constants](include/styxrpc/live_variable.h) · [Integer record](reference/value_integer.md).

For indexed I/O the client holds the selector open through access to `.val`. The value fid is released first. DisVM reserves the active selector by the mounted session. Two overlapping selections of the same collection in one session can receive a busy or denied result. Serialize their complete operations.

## Ownership and waiting

The caller owns the session and closes live bindings before destroying it. Each binding retains one exact fid and uses another walk to check visibility and identity before reading, writing, querying length or capturing a field. A field binding owns its own fid. The result of a read is a copy.

A synchronous operation can use `stop(state)` on the caller's thread. It polls between requests and during cancellable transport waits. A nonmultiplexed transport may only check stop before the request. The following outcomes have distinct uses.

| Outcome | Meaning |
| --- | --- |
| `STYXRPC_IO_OK` | Operation confirmed. Read, count and capture outputs are now valid. |
| `STYXRPC_IO_NOT_OPEN` | Admission failed. Examples include a closed or stale projection, an invalid index shape and denied modes. |
| `STYXRPC_IO_FAILED` | Confirmed I/O or record failure. |
| `STYXRPC_IO_CANCELLED` | The caller stopped the operation. Any active temporary selection is retired. |
| `STYXRPC_IO_UNCERTAIN` | A transfer lacked a reliable acknowledgement. A final write may have updated the value. Reconcile before replay. |

Other than close, each operation uses temporary fids and retires them before returning or queues their retirement after stop. The kept fid is never retargeted to a replacement publication. Use one serialized owner for a session and its handles.

## API reference

[Open](reference/live_open_until.md) · [Kind](reference/live_kind.md) · [Field](reference/live_field_until.md) · [Length](reference/live_length_until.md) · [Read](reference/live_read_until.md) · [Write](reference/live_write_until.md) · [Close](reference/live_close.md)

[Scoped channel schema with refs and tables](reference/channel_schema_valid.md) · [Leased reply API](reference/channel_invoke.md)
