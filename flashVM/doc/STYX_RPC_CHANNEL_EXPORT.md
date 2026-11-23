# fvm_styxrpc_channel_export

[Русский](STYX_RPC_CHANNEL_EXPORT.ru.md) · [Declaration](../include/flashvm/styxrpc_channel.h) · [Implementation](../src/styxrpc/channel_export.c) · [Channel guide](STYX_RPC_CHANNELS.md)

Include `flashvm/styxrpc_channel.h` and link `flashvm_styxrpc`.

```c
FVM_Result fvm_styxrpc_channel_export(FVM_StyxRpcChannelExport *binding,
    const char *name, uint32_t signature, StyxRpcChannelExport *exported);
```

Fill `binding` with the FlashVM owner, AS initializer name, complete schema and capacity. The schema has by-value arguments and one typed channel result. Capacity is 0–8 elements. A value in the initializer's result seeds one buffered slot. An undefined result leaves the channel empty and is required for a zero-capacity rendezvous.

Pass the compiled export `name` and `signature`, and provide caller-owned `exported` storage. [`FVM_OK`](../include/flashvm/flashvm.h#L44) prepares a descriptor for [channel module publication](../../lib/styxrpc/reference/channel_module_init.md). Invalid input returns [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47). Script exceptions, invalid seeds or allocation failure produce a remote error when serving a call. The initializer runs synchronously on the FlashVM context owner. Keep the binding and names stable until server destruction. Each successful function fid owns the channel group and its cancellation. A result element can describe [nested channels](STYX_RPC_NESTED_CHANNELS.md#issue-nested-channels-owned-by-flashvm) on that same lease.

[Executable AVM1 fixture](../tests/styxrpc/channel-service.c)

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
| `fvm_styxrpc_channel_export` | function | [include/flashvm/styxrpc_channel.h:53](../include/flashvm/styxrpc_channel.h#L53) |
| `FVM_StyxRpcChannelExport` | struct | [include/flashvm/styxrpc_channel.h:42](../include/flashvm/styxrpc_channel.h#L42) |
<!-- source-links:end -->
