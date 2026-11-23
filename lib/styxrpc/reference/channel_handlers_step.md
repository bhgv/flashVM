# styxrpc_channel_handlers_step

[Русский](channel_handlers_step.ru.md) · [Declaration](../include/styxrpc/channel_handlers.h) · [Implementation](../src/channel_handlers.c) · [Guide](channel_handler_modules.md) · [C test](../tests/channel_handlers.c)

Include `<styxrpc/channel_handlers.h>` and link the hosted C99 target `StyxRpc::serve` with POSIX threads.

```c
int styxrpc_channel_handlers_step(StyxRpcChannelHandlers *module);
```

Wait for one queued job in `module`, then run its callback on the calling thread. Use only one owner thread, with no recursive calls to this function. Run the forward server dispatcher separately. While a callback is executing, the dispatcher can acknowledge writes, read immutable replies and close FIDs.

Returns 1 after processing one job. Returns 0 for NULL or after the module stops. The callback may block in a reverse operation. Its arguments borrow the checked request storage until return. Output is validated and published only after the callback returns.

[`create`](channel_handlers_create.md) · [`stop`](channel_handlers_stop.md) · [`destroy`](channel_handlers_destroy.md)
