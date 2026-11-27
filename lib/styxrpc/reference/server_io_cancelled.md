# styxrpc_server_io_cancelled

[Русский](server_io_cancelled.ru.md) · [Install deferred callbacks](server_enable_deferred_io.md)

## Syntax

```c
#include <styxrpc/serve.h>

int styxrpc_server_io_cancelled(void);
```

Hosted C99, target `StyxRpc::serve`.

## Behavior

Takes no arguments. Returns `1` if the current deferred file callback's
original request has retired due to flush, fid retirement or session close.
Returns `0` before retirement and outside that callback, including on the
owner thread. The flag stays set until the callback returns. The query
performs no I/O, allocates nothing and borrows the current worker's state.

Check the flag between bounded waits inside a selected
`StyxRpcVfs.read` or `StyxRpcVfs.write` callback. The check does not interrupt
a foreign blocking call. Return once stopped. The server holds the cookie
through the return, then closes it on the owner if its fid has been retired.
A stopped callback can have applied changes before it observed the flag.

## Example

The [provider's wait loop](../examples/disvm/deferred_device.c) checks for
stop before each bounded pause, records which callback stopped and returns.
Its neighbor uses an independent fid and keeps receiving replies. Run the
[paired test](../examples/disvm/DEFERRED_IO.md) to exercise this path.

[Header](../include/styxrpc/serve.h) · [Implementation](../src/serve_deferred_io.inc)
