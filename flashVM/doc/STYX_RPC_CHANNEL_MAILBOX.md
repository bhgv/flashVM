# fvm_styxrpc_channel_mailbox

[Русский](STYX_RPC_CHANNEL_MAILBOX.ru.md) · [Declaration](../include/flashvm/styxrpc_channel.h#L135) · [Implementation](../src/styxrpc/channel_mailbox.c#L202)

```c
#include <flashvm/styxrpc_channel.h>
FVM_Result fvm_styxrpc_channel_mailbox(FVM_Context *context, const FVM_Value *anchor,
    const char *shape, unsigned slots, StyxRpcServer *reverse, FVM_Value *out);
```

Link `flashvm_styxrpc`.

Call on the owner of `context`. Use an open result channel as `anchor` and the program server on the same reverse Styx attachment as `reverse`. `shape` is a NUL-terminated one-element schema with channel leaves, such as `(c[i])`. Choose 1 through 8 `slots` per direction. The result in `out` is both an ActionScript mailbox and a PID capability. Root or assign it before the next managed allocation.

Send the mailbox as a value through a compatible channel of its result family. `tryReceive()` returns undefined if empty. It checks a dequeued frame's module tokens on the pinned module before decoding and raises an exception if the message is refused. It can wait for Styx token confirmation using the mount timeout. `trySend(value)` accepts live module channels from the same result family and returns false if the outbound FIFO is full. `close()` revokes the PID, cancels pending fids and drops unread frames. It is safe to repeat. Collected mailboxes also revoke. Keep the mailbox reachable while the peer needs its PID. The mailbox holds the lease even when the anchor wrapper is closed.

In a cancellable task context, stop interrupts [token confirmation](../../lib/styxrpc/reference/channel_lease_import_message_until.md). Cancelled confirmation raises an exception and preserves previously admitted routes. The mailbox consumes any frame it already dequeued.

[`FVM_OK`](../include/flashvm/flashvm.h#L44) publishes the mailbox. Invalid arguments or SDK creation failure give [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47). Registration failure gives [`FVM_ERROR`](../include/flashvm/flashvm.h#L45). Root or object creation errors can also propagate. Failure leaves `out` unchanged. Keep the reverse server alive through retirement. Use outside reverse dispatcher callbacks.

[Delegation guide](STYX_RPC_CHANNEL_DELEGATION.md) · [Native fixture](../tests/styxrpc/channel-mailbox.c)

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Context` | struct | [src/core/runtime.hpp:260](../src/core/runtime.hpp#L260) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_styxrpc_channel_mailbox` | function | [include/flashvm/styxrpc_channel.h:136](../include/flashvm/styxrpc_channel.h#L136) |
| `FVM_Value` | struct | [include/flashvm/value.h:26](../include/flashvm/value.h#L26) |
<!-- source-links:end -->
