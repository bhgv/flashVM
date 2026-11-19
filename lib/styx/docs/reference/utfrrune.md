# utfrrune

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](utfrrune.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>

char *utfrrune(char *s, long c);
```

## Library

Styx::nine; hosted Linux64 support

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `s` | `char *s` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `c` | `long c` | Scalar operand of this operation; protocol/algorithm ranges are described below. |

## Description

utfrrune: original POSIX lib9 utfrrune support.

### Preserved implementation/header contract

Invalid rune values must not alias a byte after libc character conversion.

## Return value

`char *`; direct result constants in this body: nil. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::nine; hosted Linux64 support

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: vendor/inferno/lib9/utfrrune.c:22](../../vendor/inferno/lib9/utfrrune.c#L22)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
