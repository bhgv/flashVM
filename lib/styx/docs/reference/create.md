# create

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](create.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>

int create(char *f, int mode, int perm);
```

## Library

Styx::nine; hosted Linux64 support

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `f` | `char *f` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `mode` | `int mode` | Requested operation/permission mode; use the documented protocol constants, not a pathname suffix. |
| `perm` | `int perm` | Scalar operand of this operation; protocol/algorithm ranges are described below. |

## Description

create: original POSIX lib9 create support.

### Preserved implementation/header contract

of loved one & descendants

## Return value

`int`; direct result constants in this body: -1. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::nine; hosted Linux64 support

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: vendor/inferno/lib9/create.c:27](../../vendor/inferno/lib9/create.c#L27)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
