# walk_leave

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](walk_leave.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/walk/walk.h>

int walk_leave(WalkScope *scope);
```

## Library

Styx::walk; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `scope` | `WalkScope *scope` | Request-local active traversal guard; successful entry requires its matching leave. |

## Description

walk_leave: walk/walk implementation and native state.

### Preserved implementation/header contract

EN/RU: Pop exactly this active scope and release imported/deep storage;1/0 on unbalanced/wrong-thread use.
A failed enter must not be left. No callbacks, I/O or managed owners.
Pop active scope, освободить imported/deep storage;1/0 при unbalanced/wrong-thread use. Без callbacks/I/O.

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

- [Implementation: walk/walk.c:285](../../walk/walk.c#L285)
- [Public declaration](../../walk/walk.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
