# charstod

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](charstod.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>

double charstod(int (*f)(void *), void *vp);
```

## Library

Styx::nine; hosted Linux64 support

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `int (*f)(void *)` | `int (*f)(void *)` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `vp` | `void *vp` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |

## Description

charstod: original POSIX lib9 charstod support.

### Preserved implementation/header contract

one-of-a-kind

## Return value

`double`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::nine; hosted Linux64 support

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: vendor/inferno/lib9/charstod.c:40](../../vendor/inferno/lib9/charstod.c#L40)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
