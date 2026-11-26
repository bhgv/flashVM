# styxrpc_cell_attach

[Русский](cell_attach.ru.md) · [Header](../include/styxrpc/named_cell.h) · [Implementation](../src/named_channel_client.c) · [Cells](named_cell.md)

Pass the issuing session and a readable cell token. The issuer returns a new owned read/write fid while the token is live. Missing or expired tokens return NULL. Attach before a channel releases its transfer pin. Link `StyxRpc::launch` and release the fid with [close](vfs_close.md).

I/O operations use the session request deadlines.
