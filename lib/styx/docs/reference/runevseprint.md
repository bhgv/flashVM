# runevseprint

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](runevseprint.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>

Rune *runevseprint(Rune *buf, Rune *e, char *fmt, va_list args);
```

## Library

Styx::nine; hosted Linux64 support

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `buf` | `Rune *buf` | Unicode rune value or rune buffer, distinct from a UTF-8 byte count. |
| `e` | `Rune *e` | Unicode rune value or rune buffer, distinct from a UTF-8 byte count. |
| `fmt` | `char *fmt` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `args` | `va_list args` | Scalar operand of this operation; protocol/algorithm ranges are described below. |

## Description

runevseprint: original POSIX lib9 runevseprint support.

### Preserved implementation/header contract

Validate aligned native Rune endpoints before pointer formation or format dispatch.

## Return value

`Rune *`; direct result constants in this body: nil. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::nine; hosted Linux64 support

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
p = runevseprint(buf, e, fmt, args);
```

[Complete calling context](../../vendor/inferno/lib9/runeseprint.c#L29)

## See also

- [Implementation: vendor/inferno/lib9/runevseprint.c:27](../../vendor/inferno/lib9/runevseprint.c#L27)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
