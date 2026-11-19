# styxmalloc

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxmalloc.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>
#include <styxserver.h>

void *styxmalloc(int bytes);
```

## Library

Styx::server; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `bytes` | `int bytes` | Scalar operand of this operation; protocol/algorithm ranges are described below. |

## Description

styxmalloc: original server session/fid dispatch and callback-backed namespace operations.

## Return value

`void *`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::server; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
void *styxmalloc(int n);
```

[Complete calling context](../../vendor/inferno/include/readable_parts/styxserver-h/part-001.inc#L306)

## See also

- [Implementation: vendor/inferno/tools/libstyx/readable_parts/styxserver-c/part-001.inc:128](../../vendor/inferno/tools/libstyx/readable_parts/styxserver-c/part-001.inc#L128)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
