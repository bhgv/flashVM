# styxrpc_named_cells_enable

[Русский](named_cells_enable.ru.md) · [Header](../include/styxrpc/named_cell.h) · [Implementation](../src/named_channel.c) · [Cells](named_cell.md)

Pass a fresh registry, an entropy callback and its optional state. Call before opening any endpoint. Return one on success. Missing inputs and repeated registration return zero. The registry borrows the state until destruction. The callback must fill the full requested extent and return one for success. It runs under the operation lock and must not reenter the registry. Link `StyxRpc::serve`.
