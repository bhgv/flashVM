# runelen

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](runelen.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>

int runelen(long c);
```

## Library

Styx::nine; hosted Linux64 support

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `c` | `long c` | Scalar operand of this operation; protocol/algorithm ranges are described below. |

## Description

runelen: UTF-8 byte/rune conversion and validity.

### Preserved implementation/header contract

rune routines

## Return value

`int`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::nine; hosted Linux64 support

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
w = runelen(c);
```

[Complete calling context](../../vendor/inferno/lib9/fmtquote.c#L95)

## See also

- [Implementation: vendor/inferno/lib9/rune.c:177](../../vendor/inferno/lib9/rune.c#L177)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
