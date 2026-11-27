# styxclientwriteuntil

[Русский](styxclientwriteuntil.ru.md) · [API index](../index.md)

Write one Styx data chunk with an optional stop callback.

## Syntax

```c
#include <styx/client/client.h>

void styxclientwriteuntil(StyxFid *fid, unsigned long long offset, const void *bytes,
    size_t size, StyxClientStop stop, void *state, StyxTransfer *result);
```

Link `Styx::client`. The API is available with the hosted native client.

| Parameter | Contract |
| --- | --- |
| `fid` | Borrow an open writable fid. Serialize operations and close on this fid. |
| `offset` | Byte offset from 0 through `2^63 - 1`. |
| `bytes`, `size` | Input of `size` readable bytes. A zero size allows a NULL buffer. The negotiated I/O unit can shorten the request. |
| `stop`, `state` | Optional [callback](../../client/readable_parts/client-h/part-002.inc#L335) returning nonzero to stop and its borrowed context. |
| `result` | Output storage for [StyxTransfer](../../client/file.h#L20). A NULL result skips the operation. |

## Completion and cancellation

Keep all arguments valid through return. The callback runs on the caller,
using the same wait and deadline rules as [styxclientreaduntil](styxclientreaduntil.md).
A serial session checks only on entry. A NULL callback gives
[styxclientwrite](styxclientwrite.md) behavior.

A matching `Rwrite` received before `Rflush` gives the acknowledged
prefix in `result->count`. When a callback stops a write before any
send attempt, `cancelled` is set with a zero count and zero uncertainty.
If the send was attempted but the original reply did not arrive, the
write may have taken effect. The client sets `uncertain`, fences
further data operations on this fid and leaves the caller owning it.
Dispose of the fid with [styxclientclunk](styxclientclunk.md).

## Example

Supply an open file, a request buffer and a callback that samples the
task's stop flag. Advance the caller's offset by the returned count
only when the transfer succeeds.

```c
StyxTransfer transfer;
styxclientwriteuntil(fid, offset, request, size, stop, task, &transfer);
if (transfer.success) {
    offset += transfer.count;
}
```

[SDK call site](https://github.com/bhgv/flashVM/blob/master/lib/styxrpc/src/channel_exchange.c)

## See also

- [Declaration](../../client/readable_parts/client-h/part-002.inc#L358) · [Implementation](../../client/io.c#L148)
- [styxclientreaduntil](styxclientreaduntil.md) · [styxclientwritecancel](styxclientwritecancel.md)
