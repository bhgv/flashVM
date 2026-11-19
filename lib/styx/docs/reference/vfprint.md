# vfprint

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](vfprint.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>

int vfprint(int fd, char *fmt, va_list args);
```

## Library

Styx::nine; hosted Linux64 support

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `fd` | `int fd` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `fmt` | `char *fmt` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `args` | `va_list args` | Scalar operand of this operation; protocol/algorithm ranges are described below. |

## Description

vfprint: original POSIX lib9 vfprint support.

### Preserved implementation/header contract

Dispatch only a valid format, then report final native write failures.

## Return value

`int`; direct result constants in this body: -1. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::nine; hosted Linux64 support

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
n = vfprint(fd, fmt, args);
```

[Complete calling context](../../vendor/inferno/lib9/fprint.c#L40)

## See also

- [Implementation: vendor/inferno/lib9/vfprint.c:115](../../vendor/inferno/lib9/vfprint.c#L115)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
