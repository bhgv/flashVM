# fvm_styxrpc_channel_handler

[Русский](STYX_RPC_CHANNEL_HANDLER.ru.md) · [Declaration](../include/flashvm/styxrpc_channel.h) · [Implementation](../src/styxrpc/channel_handler.c) · [Channel argument guide](STYX_RPC_CHANNEL_ARGUMENTS.md)

Include `<flashvm/styxrpc_channel.h>` and link `flashvm_styxrpc` in a hosted build with `FVM_ENABLE_STYXRPC=ON`.

```c
FVM_Result fvm_styxrpc_channel_handler(FVM_StyxRpcChannelHandler *binding,
    const char *name, uint32_t signature, StyxRpcChannelFunction *function);
```

| Parameter | Contract |
| --- | --- |
| `binding` | Required. `context` is the FlashVM callback owner. `function` within the binding names a global callable in that context. `reverse` is the mounted reverse client of the incoming connection. `schema` contains the exact argument and result type lists. `wait_ms` is the per-operation wait in milliseconds, 1–60000, with no default. |
| `name` | Required exact compiled Limbo export name and Styx file component. |
| `signature` | Limbo compiler's 32-bit signature for the export. |
| `function` | Required caller-owned output descriptor for [creating the hosted module](../../lib/styxrpc/reference/channel_handlers_create.md). |

Returns [`FVM_OK`](../include/flashvm/flashvm.h#L44) with a populated descriptor or [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47) leaving the output untouched. The schema must have at least one channel argument. Copied arguments and results use the managed value shapes, and the result may be void. Argument channel elements can carry copied types and [same-bus channels](STYX_RPC_NESTED_CHANNELS.md). A nil channel decodes to `null`.

Binding only prepares the descriptor. Use the FlashVM context owner for [each callback step](../../lib/styxrpc/reference/channel_handlers_step.md). The forward Styx dispatcher must continue separately while a channel method blocks. Keep the binding, names, context and reverse client alive until the server and active callback are finished. Every channel argument becomes closed at callback exit, including when the function throws or result encoding fails. Effects of completed operations remain visible in DisVM. Runtime callback failures are reported as RPC errors.

[Working host setup and callback](../tests/styxrpc/channel-incoming.c) · [Limbo caller](../../lib/styxrpc/tests/flashvm/channel_incoming.b)

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
| `fvm_styxrpc_channel_handler` | function | [include/flashvm/styxrpc_channel.h:114](../include/flashvm/styxrpc_channel.h#L114) |
| `FVM_StyxRpcChannelHandler` | struct | [include/flashvm/styxrpc_channel.h:100](../include/flashvm/styxrpc_channel.h#L100) |
<!-- source-links:end -->
