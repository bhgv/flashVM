# Positional reference graphs over Styx RPC

[Русский](STYX_RPC_GRAPH.ru.md) · [Bridge](STYX_RPC.md) · [Ordinary refs](STYX_RPC_REFS.md)

Use the graph binding for nested or returned Limbo refs. A module publishes its compiler-derived shapes in `.interface` and the native Dis marker `v00009\n` in `.ref-fields`. The client verifies both on the same held module directory before sending a call. Each call uses a fresh function fid, with a positional request (tag 112) and a sparse update list (tag 19). FlashVM and independent C programs use the same libstyxrpc codec and carrier.

| Limbo value | ActionScript representation |
| --- | --- |
| `ref ADT` | A dense native Array in declared field order, or `null` for nil. |
| `array of ref ADT` | An array of ADT Arrays or nil elements. |
| Tuple fields | An Array in tuple order. Copy-back replaces the tuple on a changed path. |
| Scalars and value arrays | The [managed mappings](STYX_RPC_IMPORT_MANAGED.md). |

The identity table is shared by arguments, result and updates for one call. Repeating a ref returns the same local Array. A new ref can be shared by the result and a changed field. An unchanged field keeps its local value, including an array's identity. The caller checks and stages the whole reply before updating writable native slots. A remote exception or local copy-back failure leaves the caller's ref slots intact. A local failure can occur after the remote function has run.

## Call Dis from FlashVM

1. Compile the Limbo module and use its export name, signature and complete argument/result shapes.
2. Mount the Dis namespace through libstyxrpc and prepare a [managed target](../../lib/styxrpc/include/styxrpc/managed_client.h). Use the host's delegated PID for a scoped native export.
3. Register [`fvm_styxrpc_import_graph`](STYX_RPC_IMPORT_GRAPH.md) in the FlashVM owner and call the global from ActionScript. Use the [ordinary managed binding](STYX_RPC_IMPORT_MANAGED.md) for functions that use the ordinary profile.

## Call FlashVM from Dis

1. Define the AS function on the server owner and create its [graph descriptor](STYX_RPC_EXPORT_GRAPH.md) using the matching Limbo interface.
2. Publish the descriptor with [`styxrpc_graph_module_init`](../../lib/styxrpc/reference/graph_module_init.md) and attach that tree to the Styx server.
3. Use the typed Limbo `load` on the mounted module path and call the export as usual.

The bridge supports frames up to 64 KiB, up to 256 ref identities per call and up to eight flattened fields in a sparse update. There are also at most eight direct fields per AS ref Array. As in the Dis codec, an alias to an incomplete definition is rejected. ActionScript mappings for hybrid tables and live channels remain open.

[Limbo interface](../../lib/styxrpc/tests/flashvm/graph.m) · [Dis owner](../../lib/styxrpc/tests/flashvm/graph_owner.b) · [Dis caller](../../lib/styxrpc/tests/flashvm/graph_client.b) · [FlashVM fixture](../tests/styxrpc/graph.c) · [Independent C client](../../lib/styxrpc/examples/client.c)

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_styxrpc_import_graph` | function | [include/flashvm/styxrpc_graph.h:16](../include/flashvm/styxrpc_graph.h#L16) |
<!-- source-links:end -->
