# styxdeferwrite

[Русский](styxdeferwrite.ru.md) · [API index](../index.md)

Keep the current `Twrite` pending while the server dispatches other tags.

```c
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>

StyxWrite *styxdeferwrite(Styxserver *server, void *aux, void (*release)(void *));
```

Available on the native hosted server targets `Styx::server` and
`Styx::phy_server`. Call only from the active write callback on the
server owner. `server` is borrowed. On success the request retains
`aux` for the optional `release(aux)` callback. The callback runs
once on retirement on the server owner, before associated fid cleanup.
It should finish without blocking or reentering the dispatcher.

A non-NULL result owns one request reference. Copy the write input
before returning from the callback, set the callback's count to zero
and return NULL. A native producer can signal the dispatcher when
the work completes. Call [styxwritereply](styxwritereply.md) and
[styxwritefree](styxwritefree.md) on the dispatcher.

A NULL result leaves `aux` with the caller. Admission requires a
nonempty write on a regular fid with no other deferred data request.
The shared limit is 128 deferred requests per client. Reset, disconnect
and `Tflush` retire the request. Clunk of the fid retires it with an
error under the same rules as [deferred reads](styxdeferread.md).

The [dispatcher fixture](../../vendor/inferno/tools/libstyx/tests/deferred-read.c)
holds a three-byte write while another fid reads. A matching
`Tflush` retires just the write. Run it with
`ctest --test-dir build -R '^styx-original-deferred-read$'`.

[Declaration](../../vendor/inferno/include/readable_parts/styxserver-h/part-002.inc#L185) ·
[Implementation](../../vendor/inferno/tools/libstyx/read.c#L108)
