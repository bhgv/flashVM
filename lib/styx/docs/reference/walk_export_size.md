# walk_export_size

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](walk_export_size.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/walk/walk.h>

size_t walk_export_size(void);
```

## Library

Styx::walk; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| — | void | No parameters. |

## Description

walk_export_size: walk/wire implementation and native state.

### Preserved implementation/header contract

EN/RU: Exact exportable current history size including header;0 if any entry has a local-only domain.
Native owner only, no allocation/I/O. Точный размер exportable current history;0 при local-only domain.

## Return value

`size_t`; direct result constants in this body: 0. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::walk; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
size_t measured = kind == WALK_PACKET_REPLY ? 4 : walk_export_size();
```

[Complete calling context](../../walk/packet.c#L94)

## See also

- [Implementation: walk/wire.c:75](../../walk/wire.c#L75)
- [Public declaration](../../walk/walk.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
