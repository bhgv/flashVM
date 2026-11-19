# walk_packet_read

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](walk_packet_read.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/walk/packet.h>

int walk_packet_read(const void *input, size_t length, WalkPacket *output);
```

## Library

Styx::walk; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `input` | `const void *input` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |
| `length` | `size_t length` | Extent/count used by the selected conversion; byte/rune/record units follow its signature and description. |
| `output` | `WalkPacket *output` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |

## Description

walk_packet_read: checked ordinal prefixes around unchanged S-record bytes.

### Preserved implementation/header contract

EN/RU: Verify complete envelope/control/data CRCs, canonical record boundaries and bounded raw 9P geometry;1/0.
input/output borrowed through return; output unchanged on failure. No history scope or operation is admitted here.
Проверить envelope/control/data CRC и canonical boundaries/raw 9P geometry;1/0, output неизменен при failure.

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

- [Implementation: walk/packet.c:164](../../walk/packet.c#L164)
- [Public declaration](../../walk/packet.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
