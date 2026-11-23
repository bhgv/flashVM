# fvm_styxrpc_channel_delegate_local

[Русский](STYX_RPC_CHANNEL_DELEGATE_LOCAL.ru.md) · [Declaration](../include/flashvm/styxrpc_channel.h#L39) · [Implementation](../src/styxrpc/channel_publication.c#L265)

Include `<flashvm/styxrpc_channel.h>` and link `flashvm_styxrpc`.

```c
FVM_Result fvm_styxrpc_channel_delegate_local(FVM_Context *context, const FVM_Value *anchor,
    const FVM_Value *value, StyxRpcServer *reverse, FVM_Value *out);
```

| Parameter | Contract |
| --- | --- |
| `context` | FlashVM context. Call on its owner, outside reverse dispatcher callbacks. |
| `anchor` | Open channel from a leased DisVM result. |
| `value` | Open queue created by [the local channel factory](STYX_RPC_CHANNEL_FACTORY.md) in this context. It may be a FIFO or rendezvous and its elements may carry channels from the same context. |
| `reverse` | [Program server](../../lib/styxrpc/reference/program_server_create.md) on the reverse half of the same Styx attachment as the anchor. The host supplies this association. |
| `out` | Required result slot for the opaque ActionScript PID value. Root or assign the result before the next managed allocation. |

Returns [`FVM_OK`](../include/flashvm/flashvm.h#L44) when published. The graph includes up to eight distinct queues discovered through occupied slots, including the root. Wrong context, closed channels, an active loan, exceeding the graph limit, or failure to prepare the queued frames produce [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47). Failure to register the PID returns [`FVM_ERROR`](../include/flashvm/flashvm.h#L45). Allocation and root errors for the managing wrapper and graph array propagate. A failed call preserves `out` and releases every reserved queue.

Send the opaque result in a matching message on the issuing family. The reverse dispatcher has exclusive access to all published queues. Local `trySend()`, `tryReceive()`, `close()` and scoped lending are unavailable to them during this period. Keep `reverse` alive until every channel wrapper and opaque PID value in the family is closed or collected. The last release revokes the PID, cancels pending fids and returns the queues, preserving completed operations. The next owner access restores exact slot references from the traced graph. Registry changes can wait for the running reverse request. GC release performs revocation using native state and avoids wire I/O.

[Register an AS function](STYX_RPC_CHANNEL_PUBLISHER.md) · [Live graph exchange with DisVM](../tests/styxrpc/channel-pid-graph.c)

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
