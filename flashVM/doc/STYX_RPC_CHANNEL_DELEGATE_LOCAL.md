# fvm_styxrpc_channel_delegate_local

[Русский](STYX_RPC_CHANNEL_DELEGATE_LOCAL.ru.md) · [Declaration](../include/flashvm/styxrpc_channel.h#L37) · [Implementation](../src/styxrpc/channel_delegate.c#L132)

Include `<flashvm/styxrpc_channel.h>` and link `flashvm_styxrpc`.

```c
FVM_Result fvm_styxrpc_channel_delegate_local(FVM_Context *context, const FVM_Value *anchor,
    const FVM_Value *value, StyxRpcServer *reverse, FVM_Value *out);
```

| Parameter | Contract |
| --- | --- |
| `context` | FlashVM context. Call on its owner, outside reverse dispatcher callbacks. |
| `anchor` | Open channel from a leased DisVM result. |
| `value` | Open queue created by [the local channel factory](STYX_RPC_CHANNEL_FACTORY.md) in this context. Its element schema must be by value and its capacity may be zero. |
| `reverse` | [Program server](../../lib/styxrpc/reference/program_server_create.md) on the reverse half of the same Styx attachment as the anchor. The host supplies this association. |
| `out` | Required result slot for the opaque ActionScript PID value. Root or assign the result before the next managed allocation. |

Returns [`FVM_OK`](../include/flashvm/flashvm.h#L44) when published. Wrong context, closed channels, an active loan, unsupported schemas or failure to prepare the borrowed queue produce [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47). Failure to register the PID returns [`FVM_ERROR`](../include/flashvm/flashvm.h#L45). Allocation and root errors for the FlashVM wrapper propagate. A failed call preserves `out` and restores any queue it began lending.

Send the opaque result in a matching message on the issuing family. The dispatcher exclusively uses the original queue while it is published. Local `trySend()`, `tryReceive()`, `close()` and scoped argument lending are unavailable during this period. Keep `reverse` alive until the last result channel wrapper and opaque PID value of the family are closed or collected. The last release revokes the PID, cancels pending fids and returns the queue, preserving completed operations. Registry changes can wait for the running reverse request. GC release does no wire I/O for the revocation.

[Register an AS function](STYX_RPC_CHANNEL_PUBLISHER.md) · [Live exchange with DisVM](../tests/styxrpc/channel-delegated.c)

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Context` | struct | [src/core/runtime.hpp:257](../src/core/runtime.hpp#L257) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_styxrpc_channel_delegate_local` | function | [include/flashvm/styxrpc_channel.h:39](../include/flashvm/styxrpc_channel.h#L39) |
| `FVM_Value` | struct | [include/flashvm/value.h:26](../include/flashvm/value.h#L26) |
<!-- source-links:end -->
