# vseprint

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](vseprint.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>

char *vseprint(char *buf, char *e, char *fmt, va_list args);
```

## Library

Styx::nine; hosted Linux64 support

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `buf` | `char *buf` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |
| `e` | `char *e` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `fmt` | `char *fmt` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `args` | `va_list args` | Scalar operand of this operation; protocol/algorithm ranges are described below. |

## Description

vseprint: original POSIX lib9 vseprint support.

### Preserved implementation/header contract

Compare native endpoint coordinates before forming the bounded output range.

## Return value

`char *`; direct result constants in this body: nil. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::nine; hosted Linux64 support

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
vseprint(errstring, errstring + sizeof(errstring), fmt, arg);
```

[Complete calling context](../../vendor/inferno/lib9/errstr-Nt.c#L79)

## See also

- [Implementation: vendor/inferno/lib9/vseprint.c:40](../../vendor/inferno/lib9/vseprint.c#L40)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
