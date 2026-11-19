# walk_entity

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](walk_entity.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/walk/walk.h>

int walk_entity(WalkEntity *output);
```

## Library

Styx::walk; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `output` | `WalkEntity *output` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |

## Description

walk_entity: walk/walk implementation and native state.

### Preserved implementation/header contract

EN/RU: Mint native root identity into borrowed output;1/0 on exhausted identity counter, no allocation/I/O.
Current configured domain is copied; zero domain remains local-only until caller supplies an explicit identity.
Создать native root identity в output;1/0 при exhausted counter, без allocation/I/O; zero domain — local-only.

## Return value

`int`; direct result constants in this body: 0. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::walk; hosted native build

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: walk/walk.c:126](../../walk/walk.c#L126)
- [Public declaration](../../walk/walk.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
