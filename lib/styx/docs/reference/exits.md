# exits

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](exits.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>

void exits(char *s);
```

## Library

Styx::nine; hosted Linux64 support

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `s` | `char *s` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |

## Description

exits: original POSIX lib9 exits support.

### Preserved implementation/header contract

of loved one & descendants

## Return value

No C return value; output/state/ownership effects are specified in Description.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::nine; hosted Linux64 support

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
exits(buf);
```

[Complete calling context](../../vendor/inferno/lib9/sysfatal.c#L32)

## See also

- [Implementation: vendor/inferno/lib9/exits.c:17](../../vendor/inferno/lib9/exits.c#L17)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
