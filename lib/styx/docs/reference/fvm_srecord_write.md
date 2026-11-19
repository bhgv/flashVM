# fvm_srecord_write

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_srecord_write.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/srecord.h>

FVM_SResult fvm_srecord_write(
    unsigned long tag, const void *payload, size_t size, unsigned char *out, size_t capacity, size_t *used);
```

## Library

Styx::wire; C89

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `tag` | `unsigned long tag` | Unsigned direction-local tag, 0..0xffffffff; transport tag 0 is service-owned. |
| `payload` | `const void *payload` | Borrowed opaque bytes, possibly nested records; disjoint from output/state. |
| `size` | `size_t size` | Payload bytes; tag+payload must fit the body profile bound. |
| `out` | `unsigned char *out` | Caller-owned writable destination for the complete checked record. |
| `capacity` | `size_t capacity` | Accessible destination bytes; insufficient space reports LIMIT. |
| `used` | `size_t *used` | Complete encoded byte count on OK; unchanged on failure. |

## Description

fvm_srecord_write: canonical integers, bounded S-record framing and checksum validation.

### Preserved implementation/header contract

Record = unsigned checksum, unsigned length, unsigned tag, payload. Length counts
tag+payload. CRC covers exact length/tag/payload bytes; profile 1 retains
6+7*(length-varint-size-1) bits, capped at 32. Integers are canonical, so checksum may use
fewer bytes if its value is small. Write validates all sizes before output. Payload/output
must not overlap; no VM/GC/SDL. Outputs and used remain unchanged on failure. Tag <= U32,
body <= BODY_MAX.

## Return value

`FVM_SResult`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::wire; C89

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
fvm_srecord_write(tag, frame, frame_size, packet + 4 + first, capacity - 4 - first, &second) == FVM_S_OK;
```

[Complete calling context](../../walk/packet.c#L133)

## See also

- [Implementation: protocol/srecord.c:362](../../protocol/srecord.c#L362)
- [Public declaration](../../include/flashvm/srecord.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
