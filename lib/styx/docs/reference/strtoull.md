# strtoull

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](strtoull.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>

uvlong strtoull(const char *nptr, char **endptr, int base);
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

strtoull: original POSIX lib9 strtoull support.

### Preserved implementation/header contract

Consume all digits and saturate only beyond the full unsigned 64-bit range.

## Return value

`uvlong`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::nine; hosted Linux64 support

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
unsigned long long removed = strtoull(command + 7, &end, 10);
```

[Complete calling context](../../examples/live.c#L199)

## See also

- [Implementation: vendor/inferno/lib9/strtoull.c:26](../../vendor/inferno/lib9/strtoull.c#L26)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
