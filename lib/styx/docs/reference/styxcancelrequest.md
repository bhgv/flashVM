# styxcancelrequest

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxcancelrequest.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/client/client.h>

void styxcancelrequest(StyxCancel *cancel);
```

## Library

Styx::client; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `cancel` | `StyxCancel *cancel` | Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract. |

## Description

styxcancelrequest: native client cancel operations and lifetime.

### Preserved implementation/header contract

EN/RU: Request sticky cancellation from a retained native capability; no protocol wait.
Запросить sticky cancellation с retained capability; без protocol wait.

## Return value

No C return value; output/state/ownership effects are specified in Description.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::client; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
styxcancelrequest(cancel->child);
```

[Complete calling context](../../client/cancel.c#L71)

## See also

- [Implementation: client/cancel.c:63](../../client/cancel.c#L63)
- [Public declaration](../../client/client.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
