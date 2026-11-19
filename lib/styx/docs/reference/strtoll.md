# strtoll

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](strtoll.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>

vlong strtoll(const char *nptr, char **endptr, int base);
```

## Library

Styx::nine; hosted Linux64 support

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `nptr` | `const char *nptr` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `endptr` | `char **endptr` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `base` | `int base` | Scalar operand of this operation; protocol/algorithm ranges are described below. |

## Description

strtoll: original POSIX lib9 strtoll support.

### Preserved implementation/header contract

Accumulate signed conversion bit patterns without host signed overflow.

## Return value

`vlong`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::nine; hosted Linux64 support

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: vendor/inferno/lib9/strtoll.c:24](../../vendor/inferno/lib9/strtoll.c#L24)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
