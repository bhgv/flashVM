# walk_enter

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](walk_enter.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/walk/walk.h>

int walk_enter(WalkScope *scope,
               const WalkEntity *entity,
               unsigned long high,
               unsigned long low,
               unsigned operation,
               const char *remaining,
               const char *detail);
```

## Library

Styx::walk; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `scope` | `WalkScope *scope` | Request-local active traversal guard; successful entry requires its matching leave. |
| `entity` | `const WalkEntity *entity` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `high` | `unsigned long high` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `low` | `unsigned long low` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `operation` | `unsigned operation` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `remaining` | `const char *remaining` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `detail` | `const char *detail` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |

## Description

walk_enter: walk/walk implementation and native state.

### Preserved implementation/header contract

EN/RU: Push exact entity/node/operation/remaining/detail state; return WALK_* and enter only on OK.
Borrow strings/entity through leave; node is an unsigned32-limb object identifier, not a bind occurrence.
Top-level invocation owns a new search; parallel native threads have independent histories.
Push exact entity/node/operation/remaining/detail; WALK_*, scope возникает лишь при OK.
Strings/entity borrowed до leave; node — unsigned32 limbs, не occurrence bind; parallel histories независимы.

## Return value

`int`; direct result constants in this body: WALK_CYCLE, WALK_INVALID, WALK_LIMIT, WALK_NOMEM, WALK_OK. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::walk; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
result = walk_enter(&entry, &entity, high, low, operation, name, mode);
```

[Complete calling context](../../walk/wire.c#L237)

## See also

- [Implementation: walk/walk.c:206](../../walk/walk.c#L206)
- [Public declaration](../../walk/walk.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
