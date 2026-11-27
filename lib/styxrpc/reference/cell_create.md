# styxrpc_cell_create

[Русский](cell_create.ru.md) · [Declaration](../include/styxrpc/named_cell.h#L46) · [Implementation](../src/named_channel_client.c#L275)

Create a cell with its first lease and a token for forwarding.

## Syntax

```c
#include <styxrpc/named_cell.h>

StyxRpcVfsFile *styxrpc_cell_create(struct StyxClient *session,
    const void *frame, size_t size, void *token);
```

## Availability

Hosted C SDK, linking `StyxRpc::launch`. Requires an attached Styx client and a provider that implements shared cells at `/.channels`. The tutorial is verified on Linux64 with DisVM.

## Parameters

| Name | Description |
| --- | --- |
| `session` | Borrowed attached client that reaches the issuing `/.channels` service. Keep it alive through return. |
| `frame` | Borrowed tagged value frame. Readable through return. |
| `size` | Frame length including its tag, 1 through `STYXRPC_NAMED_MAX` bytes. The tag determines further length constraints. |
| `token` | Writable storage for `STYXRPC_CELL_TOKEN` bytes (24). Filled on success. |

## Return value

Success returns an owned fid and fills `token`. NULL reports invalid input, refusal or an incomplete exchange. The token buffer keeps its original contents on failure.

## Behavior and lifetime

The issuer validates the complete frame and assigns a fresh token. Use `styxrpc_vfs_close` after the last local use. The server can briefly retain a created cell when a reply is lost. The failed call retires the temporary fid.

The call can block for I/O under the mounted session's request deadlines.

## Example

The attached session is supplied by the application. This sample keeps the token buffer local and releases the created fid.

```c
unsigned char token[STYXRPC_CELL_TOKEN];
const unsigned char initial[] = {STYXRPC_NAMED_BOOLEAN, 1};
StyxRpcVfsFile *owner = styxrpc_cell_create(session, initial, sizeof(initial), token);
if (owner)
{
    styxrpc_vfs_close(owner, 0);
}
```

In an exchange, keep `owner` open until a receiving participant has attached.

## See also

[Shared cells](named_cell.md) · [Value frames](cell_value.md) · [styxrpc_cell_create_until](cell_create_until.md) · [styxrpc_vfs_close](vfs_close.md) · [Runnable C and Limbo tutorial](../examples/shared-cell/README.md)
