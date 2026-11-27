# styxclientreaduntil

[Русский](styxclientreaduntil.ru.md) · [API index](../index.md)

Read one Styx data chunk from an open fid, with an optional stop callback.

## Syntax

```c
#include <styx/client/client.h>

void styxclientreaduntil(StyxFid *fid, unsigned long long offset, void *bytes,
    size_t size, StyxClientStop stop, void *state, StyxTransfer *result);
```

Link `Styx::client`. The API is available with the hosted native client.

## Parameters

| Parameter | Contract |
| --- | --- |
| `fid` | Borrow an open readable fid. Serialize data operations and close on this fid. |
| `offset` | Byte offset from 0 through `2^63 - 1`. |
| `bytes`, `size` | Writable buffer of `size` bytes. A zero size allows a NULL buffer. The negotiated I/O unit can shorten the request. |
| `stop`, `state` | Optional [callback](../../client/readable_parts/client-h/part-002.inc#L335) and borrowed context. Return nonzero to stop. Run promptly on the calling thread and leave the operation's fid alone. |
| `result` | Output storage for [StyxTransfer](../../client/file.h#L20). A NULL result skips the operation. |

## Completion and cancellation

Keep the fid, buffer and callback context alive through return. A NULL
callback gives the behavior of [styxclientread](styxclientread.md).
With a callback, the function checks it before submission and during
multiplexed reply waits. A serial session checks on entry only.

A cancellation before submission sets `cancelled` and leaves
`count` and `uncertain` zero. In a multiplexed session the client
sends one `Tflush` by tag for a transmitted request. A matching
`Rread` received before `Rflush` keeps its count and bytes. If
cancellation wins after a send attempt, `uncertain` is set and
`count` stays zero. The callback form reserves `cancelled` for a
stop requested through the callback. Ordinary timeouts and transport
errors still fail the transfer.

On success, `result->success` is set and `result->count` gives the
copied byte count. An uncertain transfer fences the fid against
further data calls, with the same offset-zero read-only directory
exception as [styxclientread](styxclientread.md). The caller still
owns the fid. Waiting for `Rflush` uses the session deadline.

## Example

The SDK wire test exercises this native operation through its VFS reader.
Its stop callback samples a flag on the calling thread. When the peer
sends a one-byte reply before `Rflush`, the caller gets a confirmed `'r'`.
With a directly opened native `fid`, the call takes this form.

```c
StyxTransfer result;
char byte = 0;
styxclientreaduntil(fid, 0, &byte, 1, stop, state, &result);
```

[Wire test](https://github.com/bhgv/flashVM/blob/master/lib/styxrpc/tests/channel_stop_wire.c#L212)

## See also

- [Declaration](../../client/readable_parts/client-h/part-002.inc#L348) · [Implementation](../../client/io.c#L140)
- [styxclientwriteuntil](styxclientwriteuntil.md) · [styxclientreadcancel](styxclientreadcancel.md)
