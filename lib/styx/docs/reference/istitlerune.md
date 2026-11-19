# istitlerune

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](istitlerune.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>

int istitlerune(Rune c);
```

## Library

Styx::nine; hosted Linux64 support

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `c` | `Rune c` | Unicode rune value or rune buffer, distinct from a UTF-8 byte count. |

## Description

istitlerune: original POSIX lib9 runetypebody-6.2.0 support.

## Return value

`int`; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::nine; hosted Linux64 support

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: vendor/inferno/lib9/readable_parts/runetypebody-6-2-0-h/part-001.inc:332](../../vendor/inferno/lib9/readable_parts/runetypebody-6-2-0-h/part-001.inc#L332)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
