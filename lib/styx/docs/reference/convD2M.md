# convD2M

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](convD2M.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/codec.h>

uint convD2M(Dir *d, uchar *buf, uint nbuf);
```

## Library

Styx::codec; C99, no OS/allocator/threads; alternatively Styx::nine in the hosted build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `d` | `Dir *d` | Input directory metadata to encode as one counted stat record. |
| `buf` | `uchar *buf` | Caller-owned writable destination; do not confuse a two-byte size prefix with a complete record. |
| `nbuf` | `uint nbuf` | Destination capacity in bytes, including the leading two-byte length. |

## Description

Encode one standard Dir record; a short buffer can receive only its two-byte required-size prefix.

Call sizeD2M first when a complete record is required. If the buffer has at least two bytes but cannot hold the complete valid record, the original API writes its required-length prefix and returns 2. That is not a complete directory entry. Zero reports an invalid record or less than two bytes of capacity. Directory read adapters must emit whole records only.

### Preserved implementation/header contract

Preserve short-buffer size reporting only for representable entries.

## Return value

`uint`; direct result constants in this body: 0. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::codec; C99, no OS/allocator/threads; alternatively Styx::nine in the hosted build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
size = convD2M(&row, bytes, sizeof(bytes));
```

[Complete calling context](../../examples/codec.c#L59)

## See also

- [Implementation: vendor/inferno/lib9/convD2M.c:93](../../vendor/inferno/lib9/convD2M.c#L93)
- [Public declaration](../../include/styx/fcall.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
