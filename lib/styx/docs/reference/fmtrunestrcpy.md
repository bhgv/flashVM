# fmtrunestrcpy

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fmtrunestrcpy.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>

int fmtrunestrcpy(Fmt *f, Rune *s);
```

## Library

Styx::nine; hosted Linux64 support

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `f` | `Fmt *f` | Active original formatter context, including its output cursor and conversion state. |
| `s` | `Rune *s` | Unicode rune value or rune buffer, distinct from a UTF-8 byte count. |

## Description

fmtrunestrcpy: original POSIX lib9 dofmt support.

### Preserved implementation/header contract

public helper routine: fmt out a null terminated rune string already in hand

## Return value

`int`; direct result constants in this body: -1. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::nine; hosted Linux64 support

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
return fmtrunestrcpy(f, s);
```

[Complete calling context](../../vendor/inferno/lib9/readable_parts/dofmt-c/part-002.inc#L169)

## See also

- [Implementation: vendor/inferno/lib9/readable_parts/dofmt-c/part-002.inc:115](../../vendor/inferno/lib9/readable_parts/dofmt-c/part-002.inc#L115)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
