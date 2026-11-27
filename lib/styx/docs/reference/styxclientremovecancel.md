# styxclientremovecancel

[Русский](styxclientremovecancel.ru.md) · [API index](../index.md) · [Remove with a normal deadline](styxclientremove.md)

```c
#include <styx/client/client.h>

int styxclientremovecancel(StyxFid *fid, StyxCancel *cancel, StyxTransfer *result);
```

Hosted C99. Link `Styx::client`.

| Parameter | Meaning |
| --- | --- |
| `fid` | Owned fid for the node to unlink. Transfer it to this call and never use it again. `NULL` is a local refusal. |
| `cancel` | Optional borrowed one-use token for a multiplexed client, valid through return. `NULL` uses the normal session deadline. |
| `result` | Optional writable `StyxTransfer`. Records the acknowledged outcome, cancellation and possible effects. The byte count is zero. |

Sends at most one `Tremove`. Returns `1` only on `Rremove`. A definite `Rerror` consumes the remote fid under 9P and needs no additional clunk. If the exchange ends without an acknowledgement, the client queues independent fid retirement while the connection is usable. A sent request without acknowledgement can have unlinked the node and sets `uncertain`. The cleanup clunks the old fid and does not resubmit removal. The call also consumes the fid on pre-cancellation.

```c
int remove_report(StyxClient *session, const char *path, StyxCancel *stop,
    StyxTransfer *out)
{
    return styxclientremovecancel(styxclientwalk(session, path), stop, out);
}
```

Use a separate walk if you need another open reference to the same object. The helper's walk uses the ordinary deadline and the token controls removal itself. See [cancellable creation](styxclientcreatecancel.md), [header](../../client/client.h) and [implementation](../../client/clunk.c).
