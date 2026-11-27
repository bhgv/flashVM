# styxwritereply

[Русский](styxwritereply.ru.md) · [API index](../index.md)

Complete one deferred write on the dispatcher.

```c
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>

int styxwritereply(StyxWrite *request, ulong count, char *error);
```

Available on the native hosted server targets `Styx::server` and
`Styx::phy_server`. Borrow a live `request` from
[styxdeferwrite](styxdeferwrite.md) after its write callback has
returned. With NULL `error`, acknowledge a prefix of `count` bytes,
up to the length of the original write. To send `Rerror`, pass
`count = 0` and an error string valid through the call.

Returns 1 when it retires the request and attempts the reply. Returns
0 for a staged, cancelled, completed or invalid request. An invalid
count leaves a still-live request available for correction. On
retirement the supplied release callback runs and may drop the caller
reference. Otherwise the caller releases it with
[styxwritefree](styxwritefree.md).

The [dispatcher fixture](../../vendor/inferno/tools/libstyx/tests/deferred-read.c)
calls `styxwritereply(request, 2, NULL)` for a three-byte request and
checks the short `Rwrite` count.

[Declaration](../../vendor/inferno/include/readable_parts/styxserver-h/part-002.inc#L194) ·
[Implementation](../../vendor/inferno/tools/libstyx/read.c#L182)
