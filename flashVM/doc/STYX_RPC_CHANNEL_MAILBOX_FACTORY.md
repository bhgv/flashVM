# fvm_styxrpc_channel_mailbox_factory

[Русский](STYX_RPC_CHANNEL_MAILBOX_FACTORY.ru.md) · [Declaration](../include/flashvm/styxrpc_channel.h#L141) · [Implementation](../src/styxrpc/channel_mailbox.c#L293)

```c
#include <flashvm/styxrpc_channel.h>
FVM_Result fvm_styxrpc_channel_mailbox_factory(FVM_Context *context, const char *global, StyxRpcServer *reverse);
```

Link `flashvm_styxrpc`.

On the owner of `context`, register `global` under a nonempty NUL-terminated name. The script function receives `(anchor, shape, slots)` and creates a [reverse mailbox](STYX_RPC_CHANNEL_MAILBOX.md). It uses the provided `reverse` program server for the same bidirectional mount. Keep that server alive while the global is callable and until its mailboxes retire.

Returns [`FVM_OK`](../include/flashvm/flashvm.h#L44) on registration, [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47) for missing arguments, or the error from native function registration. Script calls with an invalid signature fail. Refused mailbox publication raises a script exception.

[Delegation guide](STYX_RPC_CHANNEL_DELEGATION.md) · [Native fixture](../tests/styxrpc/channel-mailbox.c)

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
| `fvm_styxrpc_channel_mailbox_factory` | function | [include/flashvm/styxrpc_channel.h:141](../include/flashvm/styxrpc_channel.h#L141) |
<!-- source-links:end -->
