# fvm_styxrpc_pid_mailbox_export

[Русский](STYX_RPC_PID_MAILBOX_EXPORT.ru.md) · [Declaration](../include/flashvm/styxrpc_channel.h#L160) · [Implementation](../src/styxrpc/channel_pid_mailbox.c#L217)

```c
#include <flashvm/styxrpc_channel.h>
FVM_Result fvm_styxrpc_pid_mailbox_export(FVM_StyxRpcPidMailboxExport *binding,
    const char *name, uint32_t signature, StyxRpcChannelExport *exported);
```

Link `flashvm_styxrpc`.

Call on the FlashVM owner to fill `exported` with the compiled function `name` and exact `signature`. Keep the name, `binding`, and its reverse mount alive through module teardown. The binding supplies the context, initializer, compiled input and result schema, 1 through 8 slots per direction, and a reverse operation budget of 1 through 60000 ms. Use copied input arguments and exactly one result channel whose messages contain channel leaves. For example, `(i)` → `(c[c[i]])` exposes a mailbox of integer channels.

Register the descriptor in a [leased handler module](../../lib/styxrpc/reference/channel_handlers_create_with_results.md). Each call invokes the named ActionScript initializer with a new mailbox object followed by the decoded arguments. Save the mailbox if the owner will use it after return. The script return value is ignored. The module publishes the result token after a successful initializer. An initializer failure delivers a remote call error.

On the owner, `tryReceive()` returns undefined when empty. With a frame it verifies PID identities on the borrowed mount and materializes live channels. Operations on those channels use the native reverse bus. `trySend(value)` returns previously admitted original PID grants from this mailbox to the caller, or false on a full FIFO. The reverse bus also supports [nested channel elements](../../lib/styxrpc/reference/channel_pid_exchange.md). Send a child in a compatible element on its PID bus. A retained child can outlive a closed parent proxy. The result fid's clunk closes the mailbox and its proxies for subsequent operations. Operations already in progress can still finish. Drop reachability to release the script object. Keep the reverse mount alive while operations are possible.

The registration returns [`FVM_OK`](../include/flashvm/flashvm.h#L44) or [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47) for invalid binding, signature metadata or unsupported shape. [Native test](../tests/styxrpc/channel-pid-result.c)

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
| `fvm_styxrpc_pid_mailbox_export` | function | [include/flashvm/styxrpc_channel.h:160](../include/flashvm/styxrpc_channel.h#L160) |
| `FVM_StyxRpcPidMailboxExport` | struct | [include/flashvm/styxrpc_channel.h:147](../include/flashvm/styxrpc_channel.h#L147) |
<!-- source-links:end -->
