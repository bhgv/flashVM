# styxclientfidcancellable

[Русский](styxclientfidcancellable.ru.md) · [Styx API](../index.md) · [Client transport](styxclientcancellable.md)

```c
#include <styx/client/client.h>
int styxclientfidcancellable(StyxFid *fid);
```

Hosted C99, target `Styx::client`.

Borrow `fid` while its owner prevents concurrent clunk. Returns `1` for a fid on
a multiplexed transport and `0` for `NULL` or a serial transport. This is the
transport's fixed cancellation capability. The call performs no I/O and does
not probe the peer. It keeps ownership of the fid with the caller.

For a pinned `file`, choose the metadata wait that the transport supports. The
caller owns `token` and a writable `meta`.

```c
StyxTransfer reply;
int read = styxclientfidcancellable(file) ?
    styxclientstatcancel(file, &meta, token, &reply) :
    styxclientstat(file, &meta);
```

[Header](../../client/client.h) · [Implementation](../../client/fid.c)
