# cistrncmp

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](cistrncmp.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>

int cistrncmp(char *s1, char *s2, int n);
```

## Library

Styx::nine; hosted Linux64 support

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `s1` | `char *s1` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `s2` | `char *s2` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `n` | `int n` | Extent/count used by the selected conversion; byte/rune/record units follow its signature and description. |

## Description

cistrncmp: original POSIX lib9 cistrncmp support.

### Preserved implementation/header contract

most mem and string routines are declared by ANSI/POSIX files above

## Return value

`int`; direct result constants in this body: 0. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::nine; hosted Linux64 support

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: vendor/inferno/lib9/cistrncmp.c:24](../../vendor/inferno/lib9/cistrncmp.c#L24)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
