# styxrpc_channel_mailbox_owner_release

[Русский](channel_mailbox_owner_release.ru.md) · [Declaration](../include/styxrpc/channel_mailbox.h#L50) · [Implementation](../src/channel_mailbox.c#L273)

```c
#include <styxrpc/channel_mailbox.h>
void styxrpc_channel_mailbox_owner_release(StyxRpcChannelMailbox *mailbox);
```

Link `StyxRpc::serve`.

Consume the distinct owner reference acquired by [result creation](channel_mailbox_result_create.md). Stop using `mailbox` through that reference before calling. NULL and borrowed mailboxes from the reverse PID constructor are harmless. This call performs no wire I/O. If the module still holds its resource reference, it can continue servicing fids until clunk. The last reference frees the allocation. Call once per independent owner reference. [C test](../tests/channel_mailbox.c)
