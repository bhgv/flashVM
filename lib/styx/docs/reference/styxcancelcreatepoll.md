# styxcancelcreatepoll

[Русский](styxcancelcreatepoll.ru.md) · [Styx API](../index.md) · [Explicit stop token](styxcancelcreate.md)

```c
#include <styx/client/client.h>
StyxCancel *styxcancelcreatepoll(StyxClientStop stop, void *state);
```

Hosted C99, target `Styx::client`.

Allocate an owned token for a synchronous multiplexed operation. The caller can
supply a brief `stop(state)` callback that returns nonzero when the wait should
stop. `NULL` supplies an ordinary token. Keep borrowed `state` alive through the
operation. The collecting thread invokes the callback outside client locks.
During a fragmented walk, the same logical token applies to each fragment and a
stop remains sticky. One admitted logical operation consumes the token's one-use
admission. Release your reference with `styxcancelfree`.

Returns an owned token or `NULL` when allocation fails. The constructor performs
no wire I/O and does not start polling by itself.

```c
StyxCancel *token = styxcancelcreatepoll(stop, state);
if (token)
{
    StyxTransfer reply;
    StyxFid *fid = styxclientwalkcancel(client, "dir/file", token, &reply);
    styxclientclunklater(fid);
    styxcancelfree(token);
}
```

[Header](../../client/client.h) · [Implementation](../../client/cancel.c)
