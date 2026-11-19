# rerrstr

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](rerrstr.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>

void rerrstr(char *buf, uint nbuf);
```

## Library

Styx::nine; hosted Linux64 support

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `buf` | `char *buf` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |
| `nbuf` | `uint nbuf` | Extent/count used by the selected conversion; byte/rune/record units follow its signature and description. |

## Description

rerrstr: original POSIX lib9 rerrstr support.

## Return value

No C return value; output/state/ownership effects are specified in Description.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::nine; hosted Linux64 support

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
rerrstr(buf, sizeof buf);
```

[Complete calling context](../../vendor/inferno/lib9/errfmt.c#L35)

## See also

- [Implementation: vendor/inferno/lib9/rerrstr.c:19](../../vendor/inferno/lib9/rerrstr.c#L19)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
