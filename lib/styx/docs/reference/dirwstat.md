# dirwstat

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](dirwstat.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>

int dirwstat(char *name, Dir *d);
```

## Library

Styx::nine; hosted Linux64 support

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `name` | `char *name` | Component or schema name in this operation; the function validates its own encoding/length limits. |
| `d` | `Dir *d` | Native metadata/string references; encode fields rather than transmitting the structure layout. |

## Description

dirwstat: original POSIX lib9 dirwstat support.

### Preserved implementation/header contract

last modifier name

## Return value

`int`; direct result constants in this body: -1. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::nine; hosted Linux64 support

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: vendor/inferno/lib9/dirwstat.c:38](../../vendor/inferno/lib9/dirwstat.c#L38)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
