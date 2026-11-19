# dec64

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](dec64.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>

int dec64(uchar *out, int lim, char *in, int n);
```

## Library

Styx::nine; hosted Linux64 support

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `out` | `uchar *out` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |
| `lim` | `int lim` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `in` | `char *in` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `n` | `int n` | Extent/count used by the selected conversion; byte/rune/record units follow its signature and description. |

## Description

dec64: original POSIX lib9 u64 support.

### Preserved implementation/header contract

Decode tolerated base64 input without touching zero-capacity destinations.

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

- [Implementation: vendor/inferno/lib9/u64.c:50](../../vendor/inferno/lib9/u64.c#L50)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
