# walk_depth

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](walk_depth.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/walk/walk.h>

size_t walk_depth(void);
```

## Library

Styx::walk; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| — | void | No parameters. |

## Description

walk_depth: walk/walk implementation and native state.

### Preserved implementation/header contract

EN/RU: Current native thread's active depth,0 between operations; no allocation or side effects.
Active depth текущего native thread;0 между operations, без allocation/side effects.

## Return value

`size_t`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::walk; hosted native build

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: walk/walk.c:319](../../walk/walk.c#L319)
- [Public declaration](../../walk/walk.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
