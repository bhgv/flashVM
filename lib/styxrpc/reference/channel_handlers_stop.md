# styxrpc_channel_handlers_stop

[Русский](channel_handlers_stop.ru.md) · [Declaration](../include/styxrpc/channel_handlers.h) · [Implementation](../src/channel_handlers.c) · [Guide](channel_handler_modules.md) · [C test](../tests/channel_handlers.c)

Include `<styxrpc/channel_handlers.h>` and link the hosted C99 target `StyxRpc::serve` with POSIX threads.

```c
void styxrpc_channel_handlers_stop(StyxRpcChannelHandlers *module);
```

Stop admission to `module`, discard queued jobs and wake the owner. The function returns void. NULL is accepted. Calls are safe from the dispatcher or another host thread, and repeated stops are harmless.

Stopping does not wait for an active callback. The callback is allowed to finish, and an active FID may still read its result if the server remains connected. An open FID whose job was discarded reports a read failure. Join the owner before releasing callback state or the reverse client.

[`create`](channel_handlers_create.md) · [`step`](channel_handlers_step.md) · [`destroy`](channel_handlers_destroy.md)
