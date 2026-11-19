# pow10

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](pow10.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>

double pow10(int n);
```

## Library

Styx::nine; hosted Linux64 support

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `n` | `int n` | Extent/count used by the selected conversion; byte/rune/record units follow its signature and description. |

## Description

pow10: original POSIX lib9 pow10 support.

### Preserved implementation/header contract

Preserve reciprocal subnormals and bound work without negating INT_MIN.

## Return value

`double`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::nine; hosted Linux64 support

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
h = f * pow10(-d);
```

[Complete calling context](../../vendor/inferno/lib9/fltfmt.c#L174)

## See also

- [Implementation: vendor/inferno/lib9/pow10.c:47](../../vendor/inferno/lib9/pow10.c#L47)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
