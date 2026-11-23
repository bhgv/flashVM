# fvm_styxrpc_channel_delegate_pid

[Русский](STYX_RPC_CHANNEL_DELEGATE_PID.ru.md) · [Declaration](../include/flashvm/styxrpc_channel.h) · [Implementation](../src/styxrpc/channel_delegate.c) · [Delegation guide](STYX_RPC_CHANNEL_DELEGATION.md)

Include `<flashvm/styxrpc_channel.h>` and link the hosted `flashvm_styxrpc` target.

```c
FVM_Result fvm_styxrpc_channel_delegate_pid(FVM_Context *context, const FVM_Value *anchor,
    const StyxRpcChannelRoute *route, FVM_Value *out);
```

| Parameter | Contract |
| --- | --- |
| `context` | Owner of the live FlashVM context and its values. |
| `anchor` | Open channel from an RPC result on the exact bidirectional mount where the host published the reverse resource. |
| `route` | Host-authorized `STYXRPC_ROUTE_PID` with PID in the high half of the 32-bit space, zero token, positive 31-bit key, exact element schema, and a 0 or 1 rendezvous flag. The route is copied. |
| `out` | Required output for the opaque ActionScript value. Root or assign it before the next managed allocation. |

Returns [`FVM_OK`](../include/flashvm/flashvm.h#L44) on success. Invalid pointers, a closed or non-leased anchor, or an invalid route return [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47). Allocation and root errors propagate. The output remains unchanged on failure.

Use the value in a matching channel position of a typed message sent through the same issuing lease. The value retains that native lease until GC. It represents the host's published queue and has no channel methods of its own. The host creates and revokes the reverse publication. Keep the resource active while the peer may use it. Construct the value on the FlashVM owner. This function itself does no network I/O.

[Publish a reverse queue group](../../lib/styxrpc/reference/channel_group_reverse_output.md) · [Executable host fixture](../tests/styxrpc/channel-delegated.c)

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
| `fvm_styxrpc_channel_delegate_pid` | function | [include/flashvm/styxrpc_channel.h:27](../include/flashvm/styxrpc_channel.h#L27) |
| `FVM_Value` | struct | [include/flashvm/value.h:26](../include/flashvm/value.h#L26) |
<!-- source-links:end -->
