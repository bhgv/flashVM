# styxrpc_cell_get

[Русский](cell_get.ru.md) · [Header](../include/styxrpc/named_cell.h) · [Implementation](../src/named_channel_client.c) · [Cells](named_cell.md)

Pass an owned cell fid plus output locations for a frame and its byte count. `STYXRPC_IO_OK` transfers the complete frame. Release the frame with free. Other outcomes preserve the output locations. Serialize access to the fid and link `StyxRpc::launch`.

I/O operations use the session request deadlines.
