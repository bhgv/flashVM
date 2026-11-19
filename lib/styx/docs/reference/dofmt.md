# dofmt

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](dofmt.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>

int dofmt(Fmt *f, char *fmt);
```

## Library

Styx::nine; hosted Linux64 support

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `f` | `Fmt *f` | Active original formatter context, including its output cursor and conversion state. |
| `fmt` | `char *fmt` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |

## Description

dofmt: original POSIX lib9 dofmt support.

### Preserved implementation/header contract

format the output into f->to and return the number of characters fmted

## Return value

`int`; direct result constants in this body: -1. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::nine; hosted Linux64 support

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
n = dofmt(f, fmt);
```

[Complete calling context](../../vendor/inferno/lib9/fmtprint.c#L53)

## See also

- [Implementation: vendor/inferno/lib9/readable_parts/dofmt-c/part-001.inc:39](../../vendor/inferno/lib9/readable_parts/dofmt-c/part-001.inc#L39)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
