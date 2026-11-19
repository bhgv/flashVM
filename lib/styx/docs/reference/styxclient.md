# styxclient

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxclient.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>
#include <styxserver.h>

Client *styxclient(Styxserver *server);
```

## Library

Styx::server; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `server` | `Styxserver *server` | Original server instance and dispatcher-owned state. |

## Description

Borrow the current callback's original client context from its server.

## Return value

`Client *`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::server; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
check(styxclient(&server) != nil && styxclient(&server)->u != nil, "backend released before close");
```

[Complete calling context](../../vendor/inferno/tools/libstyx/tests/lifecycle.c#L104)

## See also

- [Implementation: vendor/inferno/tools/libstyx/readable_parts/styxserver-c/part-005.inc:239](../../vendor/inferno/tools/libstyx/readable_parts/styxserver-c/part-005.inc#L239)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
