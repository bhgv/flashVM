# qsort

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](qsort.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>

void qsort(void *va, long n, long es, int (*cmp)(void *, void *));
```

## Library

Styx::nine; hosted Linux64 support

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `va` | `void *va` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `n` | `long n` | Extent/count used by the selected conversion; byte/rune/record units follow its signature and description. |
| `es` | `long es` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `int (*cmp)(void *, void *)` | `int (*cmp)(void *, void *)` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |

## Description

qsort: original POSIX lib9 qsort support.

### Preserved implementation/header contract

Ignore empty/invalid spans before partitioning or multiplying native counts.

## Return value

No C return value; output/state/ownership effects are specified in Description.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::nine; hosted Linux64 support

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: vendor/inferno/lib9/qsort.c:220](../../vendor/inferno/lib9/qsort.c#L220)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
