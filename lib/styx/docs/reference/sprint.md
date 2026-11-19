# sprint

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](sprint.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>

int sprint(char *buf, char *fmt, ...);
```

## Library

Styx::nine; hosted Linux64 support

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `buf` | `char *buf` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |
| `fmt` | `char *fmt` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `...` | `...` | Variadic arguments matching the supplied format. |

## Description

sprint: original POSIX lib9 sprint support.

### Preserved implementation/header contract

Bound the deprecated uncounted formatter in native uintptr arithmetic.

## Return value

`int`; direct result constants in this body: -1. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::nine; hosted Linux64 support

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
sprint(p, "%2.2ux", buf[i]);
```

[Complete calling context](../../vendor/inferno/lib9/fcallfmt.c#L355)

## See also

- [Implementation: vendor/inferno/lib9/sprint.c:37](../../vendor/inferno/lib9/sprint.c#L37)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
