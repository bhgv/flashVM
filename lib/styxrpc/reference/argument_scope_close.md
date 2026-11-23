# styxrpc_argument_scope_close

[Русский](argument_scope_close.ru.md) · [Declaration](../include/styxrpc/channel_arguments.h#L52) · [Implementation](../src/channel_arguments.c) · [Scope guide](channel_argument_scopes.md)

Link `StyxRpc::launch`.

```c
#include <styxrpc/channel_arguments.h>
void styxrpc_argument_scope_close(StyxRpcArgumentScope *scope);
```

Consume the owned `scope`, or do nothing for null. If active, revoke its keys and cancel pending reverse operations before retiring their scope. Return completed effects through the original queues' updated cursors. Existing detached operation fids reject subsequent I/O until the reverse server clunks them. After return, local queue access may resume.

[C regression](../tests/channel_arguments.c)
