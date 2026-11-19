# dec32

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](dec32.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>

int dec32(uchar *dest, int ndest, char *src, int nsrc);
```

## Library

Styx::nine; hosted Linux64 support

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `dest` | `uchar *dest` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `ndest` | `int ndest` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `src` | `char *src` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `nsrc` | `int nsrc` | Scalar operand of this operation; protocol/algorithm ranges are described below. |

## Description

dec32: original POSIX lib9 u32 support.

### Preserved implementation/header contract

Decode complete base32 byte groups. Validate the whole input and capacity
before writing, so malformed text or short buffers leave dest unchanged.

## Return value

`int`; direct result constants in this body: -1. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::nine; hosted Linux64 support

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: vendor/inferno/lib9/u32.c:31](../../vendor/inferno/lib9/u32.c#L31)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
