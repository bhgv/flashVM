# fmtinstall

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fmtinstall.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>

int fmtinstall(int c, Fmts f);
```

## Library

Styx::nine; hosted Linux64 support

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `c` | `int c` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `f` | `Fmts f` | Active original formatter context, including its output cursor and conversion state. |

## Description

fmtinstall: original format conversion registry and native callback publication.

### Preserved implementation/header contract

EN: Validate c/f and publish one registry entry; callback code runs elsewhere.
Return the original success/failure status, preserving NULL's bad-format fallback.
RU: Проверить c/f и опубликовать запись; callback выполняется снаружи.
Вернуть исходный статус, сохранив замену NULL на bad-format callback.

## Return value

`int`; direct result constants in this body: -1. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::nine; hosted Linux64 support

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
fmtinstall('q', quotestrfmt);
```

[Complete calling context](../../vendor/inferno/lib9/fmtquote.c#L426)

## See also

- [Implementation: vendor/inferno/lib9/fmt.c:153](../../vendor/inferno/lib9/fmt.c#L153)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
