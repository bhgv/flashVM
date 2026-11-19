# getfields

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](getfields.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>

int getfields(char *str, char **args, int max, int mflag, char *set);
```

## Library

Styx::nine; hosted Linux64 support

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `str` | `char *str` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `args` | `char **args` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `max` | `int max` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `mflag` | `int mflag` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `set` | `char *set` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |

## Description

getfields: original POSIX lib9 getfields support.

## Return value

`int`; direct result constants in this body: 0. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::nine; hosted Linux64 support

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: vendor/inferno/lib9/getfields.c:25](../../vendor/inferno/lib9/getfields.c#L25)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
