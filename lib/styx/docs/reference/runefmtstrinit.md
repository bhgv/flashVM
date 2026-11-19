# runefmtstrinit

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](runefmtstrinit.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>

int runefmtstrinit(Fmt *f);
```

## Library

Styx::nine; hosted Linux64 support

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `f` | `Fmt *f` | Active original formatter context, including its output cursor and conversion state. |

## Description

runefmtstrinit: original POSIX lib9 vsmprint support.

### Preserved implementation/header contract

Initialize an allocated Rune formatter with a reserved terminator slot.

## Return value

`int`; direct result constants in this body: -1, 0. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::nine; hosted Linux64 support

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: vendor/inferno/lib9/vsmprint.c:246](../../vendor/inferno/lib9/vsmprint.c#L246)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
