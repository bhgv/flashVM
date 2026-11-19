# styxgetaux

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxgetaux.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>
#include <styxserver.h>

void *styxgetaux(Styxserver *server);
```

## Library

Styx::server; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `server` | `Styxserver *server` | Original server instance and dispatcher-owned state. |

## Description

styxgetaux: original server session/fid dispatch and callback-backed namespace operations.

### Preserved implementation/header contract

Borrow current fid context; set transfers non-nil ownership only into an empty slot.
No automatic cloning: unopened walk clones start empty. Rebuild all consumers.

## Return value

`void *`; direct result constants in this body: nil. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::server; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
int *id = styxgetaux(&server);
```

[Complete calling context](../../vendor/inferno/tools/libstyx/tests/fid-context.c#L124)

## See also

- [Implementation: vendor/inferno/tools/libstyx/readable_parts/styxserver-c/part-005.inc:46](../../vendor/inferno/tools/libstyx/readable_parts/styxserver-c/part-005.inc#L46)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
