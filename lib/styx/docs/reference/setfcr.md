# setfcr

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](setfcr.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>

void setfcr(ulong fcr);
```

## Library

Styx::nine; hosted Linux64 support

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `fcr` | `ulong fcr` | Scalar operand of this operation; protocol/algorithm ranges are described below. |

## Description

setfcr: original POSIX lib9 setfcr-FreeRTOS-riscv64 support.

### Preserved implementation/header contract

FSR

## Return value

No C return value; output/state/ownership effects are specified in Description.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::nine; hosted Linux64 support

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
setfcr(FPPDBL | FPRNR | FPINVAL | FPZDIV | FPUNFL | FPOVFL);
```

[Complete calling context](../../vendor/inferno/libmath/FPcontrol-Linux.c#L20)

## See also

- [Implementation: vendor/inferno/lib9/setfcr-FreeRTOS-riscv64.c:22](../../vendor/inferno/lib9/setfcr-FreeRTOS-riscv64.c#L22)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
