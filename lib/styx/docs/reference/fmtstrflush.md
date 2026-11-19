# fmtstrflush

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fmtstrflush.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>

char *fmtstrflush(Fmt *f);
```

## Library

Styx::nine; hosted Linux64 support

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `f` | `Fmt *f` | Active original formatter context, including its output cursor and conversion state. |

## Description

fmtstrflush: original POSIX lib9 fmtstr support.

### Preserved implementation/header contract

Terminate only a valid byte view, retaining caller ownership when validation fails.

## Return value

`char *`; direct result constants in this body: nil. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::nine; hosted Linux64 support

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
result = fmtstrflush(&f);
```

[Complete calling context](../../vendor/inferno/lib9/vsmprint.c#L176)

## See also

- [Implementation: vendor/inferno/lib9/fmtstr.c:33](../../vendor/inferno/lib9/fmtstr.c#L33)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
