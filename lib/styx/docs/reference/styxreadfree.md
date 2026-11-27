# styxreadfree

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxreadfree.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>
#include <styxserver.h>

void styxreadfree(StyxRead *request);
```

## Library

Styx::server; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `request` | `StyxRead *request` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |

## Description

styxreadfree: libstyx/read implementation and native state.

### Preserved implementation/header contract

Consume caller ref on owner; freeing a live handle alone does not answer/cancel it.
Поглотить caller ref на owner; само освобождение не отвечает/не отменяет запрос.

## Return value

No C return value; output/state/ownership effects are specified in Description.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::server; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
void styxreadfree(StyxRead *);
```

[Complete calling context](../../vendor/inferno/include/readable_parts/styxserver-h/part-002.inc#L175)

## See also

- [Implementation: vendor/inferno/tools/libstyx/read.c:31](../../vendor/inferno/tools/libstyx/read.c#L31)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
