# styxcancelfree

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxcancelfree.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/client/client.h>

void styxcancelfree(StyxCancel *cancel);
```

## Library

Styx::client; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `cancel` | `StyxCancel *cancel` | Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract. |

## Description

styxcancelfree: native client cancel operations and lifetime.

### Preserved implementation/header contract

EN/RU: Consume caller token ref; admitted work retains its own ref, no implicit cancel.
Поглотить caller token ref; admitted work имеет свою ref, без implicit cancel.

## Return value

No C return value; output/state/ownership effects are specified in Description.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::client; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
styxcancelfree(cancel);
```

[Complete calling context](../../client/cancel.c#L192)

## See also

- [Implementation: client/cancel.c:147](../../client/cancel.c#L147)
- [Public declaration](../../client/client.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
