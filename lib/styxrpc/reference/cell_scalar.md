# styxrpc_cell_scalar

[Русский](cell_scalar.ru.md) · [Header](../include/styxrpc/named_cell.h) · [Implementation](../src/named_cell_wire.c) · [Cells](named_cell.md)

Pass a borrowed readable frame and its byte count. Return one for a supported complete scalar and zero otherwise. The shape check covers exact fixed-width payloads and the counted text bound. It performs no I/O. Link `StyxRpc::rpc`.
