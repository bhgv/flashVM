# convS2M

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](convS2M.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/codec.h>

uint convS2M(Fcall *f, uchar *ap, uint nap);
```

## Library

Styx::codec; C99, no OS/allocator/threads; alternatively Styx::nine in the hosted build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `f` | `Fcall *f` | Input message fields and borrowed strings/data to encode; caller retains ownership. |
| `ap` | `uchar *ap` | Caller-owned output storage, disjoint from referenced input fields. |
| `nap` | `uint nap` | Available output bytes; sizeS2M reports the required complete extent. |

## Description

Encode a native Fcall as standard 9P2000 bytes with explicit little-endian fields.

f and all strings/data it references remain caller-owned. ap has nap bytes. The codec validates type and counted-field extents before writing; zero means unsupported/malformed fields or insufficient capacity. It does not allocate, send a request, advance a fid or perform a filesystem operation.

### Preserved implementation/header contract

Serialize only messages whose complete wire size has been validated.

## Return value

`uint`; direct result constants in this body: 0. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::codec; C99, no OS/allocator/threads; alternatively Styx::nine in the hosted build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
uint size = convS2M(&input, bytes, sizeof(bytes)); /* EN: Invoke convS2M with these explicit arguments. RU: Вызвать convS2M с этими явными аргументами. */
```

[Complete calling context](../../examples/codec.c#L33)

## See also

- [Implementation: vendor/inferno/lib9/readable_parts/convS2M-c/part-001.inc:330](../../vendor/inferno/lib9/readable_parts/convS2M-c/part-001.inc#L330)
- [Public declaration](../../include/styx/fcall.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
