# runetochar

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](runetochar.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>

int runetochar(char *str, Rune *rune);
```

## Library

Styx::nine; hosted Linux64 support

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `str` | `char *str` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `rune` | `Rune *rune` | Unicode rune value or rune buffer, distinct from a UTF-8 byte count. |

## Description

runetochar: UTF-8 byte/rune conversion and validity.

### Preserved implementation/header contract

rune routines

## Return value

`int`; direct result constants in this body: 1. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::nine; hosted Linux64 support

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
return runetochar(str, &rune);
```

[Complete calling context](../../vendor/inferno/lib9/rune.c#L183)

## See also

- [Implementation: vendor/inferno/lib9/rune.c:114](../../vendor/inferno/lib9/rune.c#L114)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
