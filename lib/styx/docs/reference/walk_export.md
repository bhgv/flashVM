# walk_export

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](walk_export.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/walk/walk.h>

int walk_export(void *output, size_t capacity, size_t *used);
```

## Library

Styx::walk; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `output` | `void *output` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |
| `capacity` | `size_t capacity` | Actual caller-owned output capacity; it is not permission to write past the buffer. |
| `used` | `size_t *used` | Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below. |

## Description

walk_export: walk/wire implementation and native state.

### Preserved implementation/header contract

EN/RU: Encode active scalar history into caller bytes;1 success/0 invalid/capacity/local-only identity.
used receives exact extent only on success. No pointers/layout serialized; empty history is valid.
Кодировать active scalar history в bytes;1/0 invalid/capacity/local-only, used меняется лишь при success.

## Return value

`int`; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::walk; hosted native build

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: walk/wire.c:109](../../walk/wire.c#L109)
- [Public declaration](../../walk/walk.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
