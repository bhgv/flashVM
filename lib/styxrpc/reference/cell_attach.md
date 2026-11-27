# styxrpc_cell_attach

[Русский](cell_attach.ru.md) · [Declaration](../include/styxrpc/named_cell.h#L51) · [Implementation](../src/named_channel_client.c#L296)

Attach another lease to the cell identified by a live token.

## Syntax

```c
#include <styxrpc/named_cell.h>

StyxRpcVfsFile *styxrpc_cell_attach(struct StyxClient *session, const void *token);
```

## Availability

Hosted C SDK, linking `StyxRpc::launch`. Requires an attached Styx client and a provider that implements shared cells at `/.channels`. The tutorial is verified on Linux64 with DisVM.

## Parameters

| Name | Description |
| --- | --- |
| `session` | Borrowed attached client that reaches the issuing `/.channels` service. Keep it alive through return. |
| `token` | Borrowed readable token of `STYXRPC_CELL_TOKEN` bytes (24). Keep a lease or transfer pin alive through return. |

## Return value

Success returns a new owned fid. NULL covers an invalid parameter, foreign or revoked token, unavailable service or missing acknowledgement.

## Behavior and lifetime

Attach before another named-channel request replaces the received descriptor's lifetime pin. An independently acquired fid retains its cell after the channel closes. Close it with `styxrpc_vfs_close`. Separate fids may be used independently.

The call can block for I/O under the mounted session's request deadlines.

## Example

Use the borrowed token from the same mounted registry and hold an existing lease or transfer pin.

```c
StyxRpcVfsFile *alias = styxrpc_cell_attach(session, token);
if (alias)
{
    styxrpc_vfs_close(alias, 0);
}
```

Use a token issued in this session. A copied token alone does not extend the cell's lifetime.

## See also

[Shared cells](named_cell.md) · [Value frames](cell_value.md) · [styxrpc_cell_attach_until](cell_attach_until.md) · [styxrpc_vfs_close](vfs_close.md) · [Runnable C and Limbo tutorial](../examples/shared-cell/README.md)
