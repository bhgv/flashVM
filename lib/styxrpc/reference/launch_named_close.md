# styxrpc_launch_named_close

[Русский](launch_named_close.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/launch.h>
void styxrpc_launch_named_close(struct StyxFid **request);
```

Consume the fid and clear the pointer. NULL and an already cleared pointer are accepted. Clunk requests cancellation of a still pending launch. The void result carries no execution or cancellation receipt. Serialize against status reads on the same fid.

Hosted Linux64. `StyxRpc::launch`.

[Example](../examples/disvm/named.c) · [Source](../src/launch.c)
