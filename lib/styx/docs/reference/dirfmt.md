# dirfmt

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](dirfmt.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>
#include <fcall.h>

int dirfmt(Fmt *fmt);
```

## Library

Styx::nine; hosted Linux64 support

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `fmt` | `Fmt *fmt` | Active original formatter context, including its output cursor and conversion state. |

## Description

dirfmt: original POSIX lib9 fcallfmt support.

## Return value

`int`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::nine; hosted Linux64 support

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: vendor/inferno/lib9/fcallfmt.c:240](../../vendor/inferno/lib9/fcallfmt.c#L240)
- [Public declaration](../../vendor/inferno/include/fcall.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
