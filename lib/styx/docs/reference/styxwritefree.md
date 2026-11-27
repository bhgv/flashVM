# styxwritefree

[Русский](styxwritefree.ru.md) · [API index](../index.md)

Drop the caller's reference to a deferred write on the server owner.

```c
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>

void styxwritefree(StyxWrite *request);
```

Available with `Styx::server` and `Styx::phy_server`. Pass the owned
`request` returned by [styxdeferwrite](styxdeferwrite.md), or NULL.
The dispatcher retains a reference while the wire request remains
live. Complete it with [styxwritereply](styxwritereply.md) or let
normal protocol retirement finish it. The caller can drop its
reference from the retirement callback after the request detaches.

```c
if (styxwritereply(request, accepted_bytes, NULL)) {
    styxwritefree(request);
}
```

This form assumes the retirement callback leaves the caller reference
to this code. Keep the reference through a retry when correcting an
invalid count. [Dispatcher fixture](../../vendor/inferno/tools/libstyx/tests/deferred-read.c)

[Declaration](../../vendor/inferno/include/readable_parts/styxserver-h/part-002.inc#L199) ·
[Implementation](../../vendor/inferno/tools/libstyx/read.c#L189)
