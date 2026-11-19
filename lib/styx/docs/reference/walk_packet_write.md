# walk_packet_write

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](walk_packet_write.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/walk/packet.h>

void *walk_packet_write(
    unsigned kind, unsigned long tag, const WalkEntity *root, const void *frame, size_t frame_size, size_t *length);
```

## Library

Styx::walk; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `kind` | `unsigned kind` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `tag` | `unsigned long tag` | Identifier scoped to this direction/namespace; outer0 is reserved for service traffic. |
| `root` | `const WalkEntity *root` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `frame` | `const void *frame` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `frame_size` | `size_t frame_size` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `length` | `size_t *length` | Extent/count used by the selected conversion; byte/rune/record units follow its signature and description. |

## Description

walk_packet_write: checked ordinal prefixes around unchanged S-record bytes.

### Preserved implementation/header contract

EN/RU: Build owned transport envelope/NULL with little-endian size and two checked S-records; caller frees result.
Request captures this caller's active history now; reply advertises borrowed nonzero root and empty history.
Frame/tag/root inputs borrowed through return; no provider callbacks/VM/code execution. length written on success only.
Создать owned envelope/NULL с size и двумя checked S-records; caller освобождает. Request снимает active history,
reply объявляет nonzero root и пустую history; inputs borrowed до return, length лишь при success.

## Return value

`void *`; direct result constants in this body: NULL. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::walk; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
walk_packet_write(WALK_PACKET_REQUEST, mux->walk_tag, NULL, beat->bytes, sizeof(beat->bytes), &encoded);
```

[Complete calling context](../../client/heartbeat.c#L123)

## See also

- [Implementation: walk/packet.c:83](../../walk/packet.c#L83)
- [Public declaration](../../walk/packet.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
