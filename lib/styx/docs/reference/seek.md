# seek

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](seek.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>

vlong seek(int fd, vlong where, int from);
```

## Library

Styx::nine; hosted Linux64 support

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `fd` | `int fd` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `where` | `vlong where` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `from` | `int from` | Scalar operand of this operation; protocol/algorithm ranges are described below. |

## Description

seek: original POSIX lib9 seek support.

### Preserved implementation/header contract

Keep the full signed offset when calling the Windows CRT seek entry point.

## Return value

`vlong`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::nine; hosted Linux64 support

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
previous = seek(fd, 0, 1);
```

[Complete calling context](../../vendor/inferno/lib9/pread-Nt.c#L37)

## See also

- [Implementation: vendor/inferno/lib9/seek.c:26](../../vendor/inferno/lib9/seek.c#L26)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
