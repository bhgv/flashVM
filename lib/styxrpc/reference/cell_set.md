# styxrpc_cell_set

[Русский](cell_set.ru.md) · [Header](../include/styxrpc/named_cell.h) · [Implementation](../src/named_channel_client.c) · [Cells](named_cell.md)

Pass an owned cell fid, a borrowed complete scalar frame and its byte count. `STYXRPC_IO_OK` acknowledges an atomic replacement. Invalid input is rejected before sending. An uncertain delivery may already have changed the value. Inspect it before another write. Serialize access to the fid and link `StyxRpc::launch`.

I/O operations use the session request deadlines.
