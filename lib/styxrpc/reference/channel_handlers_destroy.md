# styxrpc_channel_handlers_destroy

[Русский](channel_handlers_destroy.ru.md) · [Declaration](../include/styxrpc/channel_handlers.h) · [Implementation](../src/channel_handlers.c) · [Guide](channel_handler_modules.md) · [C test](../tests/channel_handlers.c)

Include `<styxrpc/channel_handlers.h>` and link the hosted C99 target `StyxRpc::serve` with POSIX threads.

```c
void styxrpc_channel_handlers_destroy(StyxRpcChannelHandlers *module);
```

Free the owned `module` and its synchronization resources. The function returns void. NULL is accepted. Stop the module and wait for both the owner and the Styx server to exit, including closure of every open FID, before calling destroy. Descriptor storage and callback contexts belong to the application.

[`create`](channel_handlers_create.md) · [`step`](channel_handlers_step.md) · [`stop`](channel_handlers_stop.md)
