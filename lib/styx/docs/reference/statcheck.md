# statcheck

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](statcheck.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/codec.h>

int statcheck(uchar *buf, uint nbuf);
```

## Library

Styx::codec; C99, no OS/allocator/threads; alternatively Styx::nine in the hosted build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `buf` | `uchar *buf` | Counted stat record to validate structurally. |
| `nbuf` | `uint nbuf` | Accessible extent; must contain the complete counted record. |

## Description

Validate a complete encoded stat record and each counted string without exposing a partial record.

Return 0 for one valid complete stat record, -1 for malformed extent/counts. This is structural validation, not an rwx/authentication decision and not a check that the backend object still exists.

### Preserved implementation/header contract

Validate every counted string before advancing beyond the available record.

## Return value

`int`; direct result constants in this body: -1. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::codec; C99, no OS/allocator/threads; alternatively Styx::nine in the hosted build

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: vendor/inferno/lib9/convM2D.c:23](../../vendor/inferno/lib9/convM2D.c#L23)
- [Public declaration](../../include/styx/fcall.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
