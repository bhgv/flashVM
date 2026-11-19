# walk_import

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](walk_import.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/walk/walk.h>

int walk_import(WalkScope *scope, const void *input, size_t size);
```

## Library

Styx::walk; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `scope` | `WalkScope *scope` | Request-local active traversal guard; successful entry requires its matching leave. |
| `input` | `const void *input` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |
| `size` | `size_t size` | Requested byte extent; the relevant buffer must cover it. |

## Description

walk_import: walk/wire implementation and native state.

### Preserved implementation/header contract

EN/RU: Append a validated copied remote history to this native thread; WALK_*; owned scope only on OK.
Borrow bytes through return only. Reject cycles/malformed/bounds before any provider callback, unwind on failure.
Добавить validated copied remote history; WALK_*, scope лишь при OK; bytes borrowed до return.
Cycles/malformed/bounds отказать до provider callback; rollback при failure.

## Return value

`int`; direct result constants in this body: WALK_INVALID, WALK_LIMIT, WALK_NOMEM, WALK_OK. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::walk; hosted native build

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: walk/wire.c:174](../../walk/wire.c#L174)
- [Public declaration](../../walk/walk.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
