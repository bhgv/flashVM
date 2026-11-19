# utfnlen

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](utfnlen.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>

int utfnlen(char *s, long m);
```

## Library

Styx::nine; hosted Linux64 support

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `s` | `char *s` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `m` | `long m` | Scalar operand of this operation; protocol/algorithm ranges are described below. |

## Description

utfnlen: original POSIX lib9 utfnlen support.

### Preserved implementation/header contract

Count complete runes without forming an end pointer beyond the actual string.

## Return value

`int`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::nine; hosted Linux64 support

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
return _fmtcpy(f, s, utfnlen(s, i), i); /* byte precision omits incomplete runes */
```

[Complete calling context](../../vendor/inferno/lib9/readable_parts/dofmt-c/part-002.inc#L68)

## See also

- [Implementation: vendor/inferno/lib9/utfnlen.c:22](../../vendor/inferno/lib9/utfnlen.c#L22)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
