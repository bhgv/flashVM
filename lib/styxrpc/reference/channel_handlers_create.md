# styxrpc_channel_handlers_create

[Русский](channel_handlers_create.ru.md) · [Declaration](../include/styxrpc/channel_handlers.h) · [Implementation](../src/channel_handlers.c) · [Guide](channel_handler_modules.md) · [C test](../tests/channel_handlers.c) · [Module with result channels](channel_handlers_create_with_results.md)

Include `<styxrpc/channel_handlers.h>` and link the hosted C99 target `StyxRpc::serve` with POSIX threads.

```c
StyxRpcChannelHandlers *styxrpc_channel_handlers_create(const char *name,
    const StyxRpcChannelFunction *functions, size_t count);
```

Create a hosted function tree and its serial callback queue.

| Parameter | Contract |
| --- | --- |
| `name` | Required module component under `mounted`. Provide a nonempty name up to 255 bytes without slashes. |
| `functions` | Required immutable array with unique exact compiled names, 32-bit Limbo signatures, schemas, callbacks and optional callback contexts. Each schema requires at least one channel argument. Argument elements can carry current-bus channels. The copied result uses value types. Void is allowed. |
| `count` | Number of descriptors, from 1 to `STYXRPC_MAX_EXPORTS`. |

Returns the owned module, or NULL for invalid descriptors, an interface exceeding its size limit, or unavailable memory or thread resources. The module borrows all names, descriptors and callback contexts until destroyed. A callback writes typed result children into the supplied buffer and sets the byte count on `STYXRPC_OK`. Failure produces a remote error. The queue holds up to 32 complete pending jobs plus one active callback. Each function FID accepts one complete request up to `STYXRPC_MANAGED_LIMIT` bytes. A full queue refuses the final write.

[`step`](channel_handlers_step.md) · [`stop`](channel_handlers_stop.md) · [`destroy`](channel_handlers_destroy.md)
