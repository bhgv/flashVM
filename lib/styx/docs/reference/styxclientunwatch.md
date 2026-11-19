# styxclientunwatch

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxclientunwatch.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/client/client.h>

void styxclientunwatch(StyxWatch *watch);
```

## Library

Styx::client; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `watch` | `StyxWatch *watch` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |

## Description

styxclientunwatch: native client life operations and lifetime.

### Preserved implementation/header contract

EN/RU: Consume watch without waiting; an already admitted callback may finish.
Поглотить watch без ожидания; admitted callback может завершиться позже.

## Return value

No C return value; output/state/ownership effects are specified in Description.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::client; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
void styxclientunwatch(StyxWatch *watch);
```

[Complete calling context](../../client/readable_parts/client-h/part-002.inc#L132)

## See also

- [Implementation: client/readable_parts/life-c/part-001.inc:178](../../client/readable_parts/life-c/part-001.inc#L178)
- [Public declaration](../../client/client.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
