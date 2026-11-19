# print

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](print.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>

int print(char *fmt, ...);
```

## Library

Styx::nine; hosted Linux64 support

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `fmt` | `char *fmt` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `...` | `...` | Variadic arguments matching the supplied format. |

## Description

print: original POSIX lib9 print support.

## Return value

`int`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::nine; hosted Linux64 support

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
print("%d\n", ntohs(address.sin_port));
```

[Complete calling context](../../examples/server.c#L370)

## See also

- [Implementation: vendor/inferno/lib9/print.c:32](../../vendor/inferno/lib9/print.c#L32)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
