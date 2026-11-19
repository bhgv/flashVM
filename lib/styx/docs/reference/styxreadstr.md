# styxreadstr

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxreadstr.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>
#include <styxserver.h>

long styxreadstr(ulong off, char *buf, ulong n, char *str);
```

## Library

Styx::server; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `off` | `ulong off` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `buf` | `char *buf` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |
| `n` | `ulong n` | Extent/count used by the selected conversion; byte/rune/record units follow its signature and description. |
| `str` | `char *str` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |

## Description

styxreadstr: original server session/fid dispatch and callback-backed namespace operations.

## Return value

`long`; direct result constants in this body: 0. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::server; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
*count = offset >= (vlong)(sizeof(contents) - 1) ? 0 : styxreadstr(offset, bytes, *count, (char *)contents);
```

[Complete calling context](../../examples/memory_phy/server.c#L37)

## See also

- [Implementation: vendor/inferno/tools/libstyx/readable_parts/styxserver-c/part-005.inc:326](../../vendor/inferno/tools/libstyx/readable_parts/styxserver-c/part-005.inc#L326)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
