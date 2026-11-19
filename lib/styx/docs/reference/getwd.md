# getwd

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](getwd.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>

char *getwd(char *buf, int size);
```

## Library

Styx::nine; hosted Linux64 support

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `buf` | `char *buf` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |
| `size` | `int size` | Requested byte extent; the relevant buffer must cover it. |

## Description

getwd: original POSIX lib9 getwd-Nt support.

## Return value

`char *`; direct result constants in this body: 0. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::nine; hosted Linux64 support

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: vendor/inferno/lib9/getwd-Nt.c:20](../../vendor/inferno/lib9/getwd-Nt.c#L20)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
