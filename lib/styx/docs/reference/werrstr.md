# werrstr

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](werrstr.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>

void werrstr(char *fmt, ...);
```

## Library

Styx::nine; hosted Linux64 support

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `fmt` | `char *fmt` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `...` | `...` | Variadic arguments matching the supplied format. |

## Description

werrstr: original POSIX lib9 errstr-Nt support.

## Return value

No C return value; output/state/ownership effects are specified in Description.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::nine; hosted Linux64 support

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
werrstr(Enovmem);
```

[Complete calling context](../../vendor/inferno/lib9/dirstat-Nt.c#L43)

## See also

- [Implementation: vendor/inferno/lib9/errstr-Nt.c:74](../../vendor/inferno/lib9/errstr-Nt.c#L74)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
