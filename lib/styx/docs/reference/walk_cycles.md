# walk_cycles

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](walk_cycles.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/walk/walk.h>

unsigned long walk_cycles(void);
```

## Library

Styx::walk; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| — | void | No parameters. |

## Description

walk_cycles: walk/walk implementation and native state.

### Preserved implementation/header contract

EN/RU: Current thread's cycle observation serial, no allocation/I/O or scope mutation; diagnostic only.
Serial циклов current thread, без allocation/I/O/scope mutation; только diagnostic.

## Return value

`unsigned long`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::walk; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
context->walk_before = walk_cycles();
```

[Complete calling context](../../server/phy/lifetime.c#L225)

## See also

- [Implementation: walk/walk.c:338](../../walk/walk.c#L338)
- [Public declaration](../../walk/walk.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
