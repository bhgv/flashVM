# fvm_styxrpc_channel_handler_export

[Русский](STYX_RPC_CHANNEL_HANDLER_EXPORT.ru.md) · [Declaration](../include/flashvm/styxrpc_channel.h) · [Implementation](../src/styxrpc/channel_handler.c) · [Channel argument guide](STYX_RPC_CHANNEL_ARGUMENTS.md)

Include `<flashvm/styxrpc_channel.h>` and link `flashvm_styxrpc` in a hosted build with `FVM_ENABLE_STYXRPC=ON`.

```c
FVM_Result fvm_styxrpc_channel_handler_export(FVM_StyxRpcChannelHandlerExport *binding,
    const char *name, uint32_t signature, StyxRpcChannelExport *exported);
```

| Parameter | Contract |
| --- | --- |
| `binding` | Required. `handler` supplies the owner context, callable name, mounted reverse client, compiled schema, and a wait of 1–60000 milliseconds per reverse operation. The schema has at least one channel argument and exactly one channel result. `capacity` is 0–8 FIFO slots, with zero selecting a rendezvous. |
| `name` | Required compiled export name and Styx file component. |
| `signature` | Exact 32-bit signature from the Limbo compiler. |
| `exported` | Caller-owned descriptor filled on success. Pass it to [the owner-driven SDK module](../../lib/styxrpc/reference/channel_handlers_create_with_results.md). |

Returns [`FVM_OK`](../include/flashvm/flashvm.h#L44) with a populated descriptor or [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47) with the output unchanged. Argument channels and the result channel have copied element types. The callback returns one initial element for the new FIFO or `undefined` for an empty FIFO or rendezvous. A script exception, an invalid element, or publication failure produces an RPC error.

Execute the callback on the FlashVM owner. Run the forward Styx server separately. Keep the binding, names, context and same-session reverse client alive through both threads' shutdown. Each argument object closes at callback exit. The result has its own resource and lease, which the dispatcher publishes on the forward module's `.unibus` and retires when the issuing function fid is clunked.

[Executable FlashVM host](../tests/styxrpc/channel-incoming.c) · [Limbo caller](../../lib/styxrpc/tests/flashvm/channel_incoming.b) · [Copied-result callback](STYX_RPC_CHANNEL_HANDLER.md)

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_styxrpc_channel_handler_export` | function | [include/flashvm/styxrpc_channel.h:84](../include/flashvm/styxrpc_channel.h#L84) |
| `FVM_StyxRpcChannelHandlerExport` | struct | [include/flashvm/styxrpc_channel.h:73](../include/flashvm/styxrpc_channel.h#L73) |
<!-- source-links:end -->
