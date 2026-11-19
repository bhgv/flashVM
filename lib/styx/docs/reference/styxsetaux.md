# styxsetaux

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxsetaux.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>
#include <styxserver.h>

int styxsetaux(Styxserver *server, void *aux);
```

## Library

Styx::server; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `server` | `Styxserver *server` | Original server instance and dispatcher-owned state. |
| `aux` | `void *aux` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |

## Description

styxsetaux: original server session/fid dispatch and callback-backed namespace operations.

### Preserved implementation/header contract

Borrow current fid context; set transfers non-nil ownership only into an empty slot.
No automatic cloning: unopened walk clones start empty. Rebuild all consumers.

## Return value

`int`; direct result constants in this body: -1, 0. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::server; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
check(styxsetaux(&server, id) == 0, "context adoption");
```

[Complete calling context](../../vendor/inferno/tools/libstyx/tests/fid-context.c#L97)

## See also

- [Implementation: vendor/inferno/tools/libstyx/readable_parts/styxserver-c/part-005.inc:17](../../vendor/inferno/tools/libstyx/readable_parts/styxserver-c/part-005.inc#L17)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
