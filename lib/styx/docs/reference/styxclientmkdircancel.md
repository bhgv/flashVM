# styxclientmkdircancel

[Русский](styxclientmkdircancel.ru.md) · [Styx API](../index.md) · [Ordinary mkdir](styxclientmkdir.md)

```c
#include <styx/client/client.h>
int styxclientmkdircancel(StyxFid *directory, const char *name, unsigned permissions,
    StyxCancel *cancel, StyxTransfer *result);
```

Hosted C99, target `Styx::client`.

| Parameter | Meaning |
| --- | --- |
| `directory` | Caller-owned unopened parent fid. Serialize use and cleanup. |
| `name` | Borrowed single component shorter than `STYX_NAME_MAX`, excluding dot, dotdot and slash. |
| `permissions` | Requested directory permission mask within octal `0777`. |
| `cancel` | Borrowed optional one-use token for the multiplexed transport. `NULL` uses the ordinary deadline. |
| `result` | Required writable native result. The count is zero. |

Sends one `Tcreate` with the directory bit and read mode. Returns `1` on
`Rcreate` and leaves the same fid open on the new directory. A local refusal,
`Rerror`, cancellation or a lost reply returns `0`. A sent request with a
missing original reply sets `uncertain` and fences subsequent opens and data
operations on that fid. The caller still owns the fid and must retire it.

```c
StyxFid *parent = styxclientwalk(client, ".");
StyxTransfer result;
int made = styxclientmkdircancel(parent, "folder", 0777, token, &result);
styxclientclunklater(parent);
```

[Polling token](styxcancelcreatepoll.md) · [Header](../../client/client.h) · [Implementation](../../client/open.c)
