# styxrpc_channel_result

[Русский](channel_result.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/channel.h>
StyxRpcStatus styxrpc_channel_result(const StyxRpcChannelGrant *grants, size_t grant_count,
    const StyxRpcSelection *operation, const void *bytes, size_t size, StyxRpcChannelResult *result);
```

Decode the complete `bytes` frame of length `size` with the original `operation`, `grants` and `grant_count`. On `STYXRPC_OK`, `result->selected` is the one-based guard ordinal, or zero for idle. An `int` receive sets `result->integer` for both scalar and mixed typed replies. A typed receive also retains its complete element in the borrowed `result->value` and `result->size`, valid until the reply buffer is reused. A send or idle reply has no received element.

`STYXRPC_REMOTE_ERROR` means the peer refused the operation. `STYXRPC_INVALID` means the input or reply was invalid. Both preserve the caller's `result`.

C99. `StyxRpc::rpc`.

[Example](../tests/channel_socket.c) · [Source](../src/channel.c)
