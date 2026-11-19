# qunlock

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](qunlock.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>

void qunlock(QLock *q);
```

## Library

Styx::nine; hosted Linux64 support

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `q` | `QLock *q` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |

## Description

qunlock: original POSIX lib9 lock support.

### Preserved implementation/header contract

flag

## Return value

No C return value; output/state/ownership effects are specified in Description.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::nine; hosted Linux64 support

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
qunlock(&l->x);
```

[Complete calling context](../../vendor/inferno/lib9/lock.c#L190)

## See also

- [Implementation: vendor/inferno/lib9/lock.c:152](../../vendor/inferno/lib9/lock.c#L152)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
