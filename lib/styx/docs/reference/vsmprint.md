# vsmprint

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](vsmprint.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>

char *vsmprint(char *fmt, va_list args);
```

## Library

Styx::nine; hosted Linux64 support

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `fmt` | `char *fmt` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `args` | `va_list args` | Scalar operand of this operation; protocol/algorithm ranges are described below. |

## Description

vsmprint: original POSIX lib9 vsmprint support.

### Preserved implementation/header contract

Return a complete allocated string, releasing the owner on every formatting failure.

## Return value

`char *`; direct result constants in this body: nil. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::nine; hosted Linux64 support

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
p = vsmprint(fmt, args);
```

[Complete calling context](../../vendor/inferno/lib9/smprint.c#L38)

## See also

- [Implementation: vendor/inferno/lib9/vsmprint.c:158](../../vendor/inferno/lib9/vsmprint.c#L158)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
