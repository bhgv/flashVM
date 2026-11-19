# getfsr

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](getfsr.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>

ulong getfsr(void);
```

## Library

Styx::nine; hosted Linux64 support

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| — | void | No parameters. |

## Description

getfsr: original POSIX lib9 setfcr-FreeRTOS-riscv64 support.

## Return value

`ulong`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::nine; hosted Linux64 support

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
vv = (getfsr() & ~FPFSR) | (v & FPFSR);
```

[Complete calling context](../../vendor/inferno/lib9/setfcr-Solaris-sparc.c#L77)

## See also

- [Implementation: vendor/inferno/lib9/setfcr-FreeRTOS-riscv64.c:71](../../vendor/inferno/lib9/setfcr-FreeRTOS-riscv64.c#L71)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
