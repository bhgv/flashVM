# styxrpc_channel_mailbox_active

[Русский](channel_mailbox_active.ru.md) · [Declaration](../include/styxrpc/channel_mailbox.h#L45) · [Implementation](../src/channel_mailbox.c#L259)

```c
#include <styxrpc/channel_mailbox.h>
int styxrpc_channel_mailbox_active(StyxRpcChannelMailbox *mailbox);
```

Link `StyxRpc::serve`.

Return 1 while the resource may be used, or 0 for NULL and after retirement. The pending-publication state is active. Borrow `mailbox` through the check and keep the appropriate owner or resource reference alive. Uses the mailbox mutex and performs no wire I/O. A concurrent clunk can retire the resource immediately after the call. See [result creation](channel_mailbox_result_create.md) and the [C test](../tests/channel_mailbox.c).
