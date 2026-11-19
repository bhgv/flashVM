# styxqid

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxqid.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>
#include <styxserver.h>

Qid styxqid(Path path, int isdir);
```

## Library

Styx::server; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `path` | `Path path` | Selected object path or stable identity according to the declared type; no implicit authority is granted. |
| `isdir` | `int isdir` | Scalar operand of this operation; protocol/algorithm ranges are described below. |

## Description

Construct a scalar Qid from a stable backend identity and directory flag.

## Return value

`Qid`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::server; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
out->qid = styxqid(row->id, (row->mode & CATALOG_DIR) != 0);
```

[Complete calling context](../../examples/server.c#L56)

## See also

- [Implementation: vendor/inferno/tools/libstyx/readable_parts/styxserver-c/part-005.inc:355](../../vendor/inferno/tools/libstyx/readable_parts/styxserver-c/part-005.inc#L355)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
