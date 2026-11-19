# runeseprint

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](runeseprint.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>

Rune *runeseprint(Rune *buf, Rune *e, char *fmt, ...);
```

## Library

Styx::nine; hosted Linux64 support

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `buf` | `Rune *buf` | Unicode rune value or rune buffer, distinct from a UTF-8 byte count. |
| `e` | `Rune *e` | Unicode rune value or rune buffer, distinct from a UTF-8 byte count. |
| `fmt` | `char *fmt` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `...` | `...` | Variadic arguments matching the supplied format. |

## Description

runeseprint: original POSIX lib9 runeseprint support.

## Return value

`Rune *`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::nine; hosted Linux64 support

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: vendor/inferno/lib9/runeseprint.c:23](../../vendor/inferno/lib9/runeseprint.c#L23)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
