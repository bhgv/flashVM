# fvm_styxrpc_channel_publisher

[Русский](STYX_RPC_CHANNEL_PUBLISHER.ru.md) · [Declaration](../include/flashvm/styxrpc_channel.h#L45) · [Implementation](../src/styxrpc/channel_delegate.c#L196)

Include `<flashvm/styxrpc_channel.h>` and link `flashvm_styxrpc`.

```c
FVM_Result fvm_styxrpc_channel_publisher(FVM_Context *context, const char *global, StyxRpcServer *reverse);
```

Install a global native function in `context` under the nonempty NUL-terminated name `global`. Run on the FlashVM owner. Pass the [program server](../../lib/styxrpc/reference/program_server_create.md) on the reverse half of the Styx attachment as `reverse`. Borrow that server while the function is callable and while any resulting publications exist.

Returns [`FVM_OK`](../include/flashvm/flashvm.h#L44) on registration, [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47) for missing arguments and the registrar's status on allocation or binding failure. Calling the resulting AS function with `(anchor, queue)` runs [local delegation](STYX_RPC_CHANNEL_DELEGATE_LOCAL.md). A valid call returns an opaque PID value. Refused queues or exhausted PID slots raise an AS exception. Remote use starts when the script sends the value.

The [host fixture](../tests/styxrpc/channel-delegated.c) registers `publishPort` before running the exchange. [Delegation guide](STYX_RPC_CHANNEL_DELEGATION.md)

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Context` | struct | [src/core/runtime.hpp:257](../src/core/runtime.hpp#L257) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_styxrpc_channel_publisher` | function | [include/flashvm/styxrpc_channel.h:47](../include/flashvm/styxrpc_channel.h#L47) |
<!-- source-links:end -->
