# styxrpc_cell_create

[Русский](cell_create.ru.md) · [Header](../include/styxrpc/named_cell.h) · [Implementation](../src/named_channel_client.c) · [Cells](named_cell.md)

Pass an attached session, a complete scalar frame and its byte count, and writable storage for a cell token. On confirmation return an owned read/write fid and fill the token. On refusal or a lost reply return NULL and preserve the output token. Link `StyxRpc::launch` and release the fid with [close](vfs_close.md).

I/O operations use the session request deadlines.
