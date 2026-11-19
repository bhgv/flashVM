# fvm_srecord_write_pair

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_srecord_write_pair.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/srecord.h>

FVM_SResult fvm_srecord_write_pair(unsigned long tag,
                                   const void *first,
                                   size_t first_size,
                                   const void *second,
                                   size_t second_size,
                                   unsigned char *out,
                                   size_t capacity,
                                   size_t *used);
```

## Library

Styx::wire; C89

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `tag` | `unsigned long tag` | Identifier scoped to this direction/namespace; outer0 is reserved for service traffic. |
| `first` | `const void *first` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `first_size` | `size_t first_size` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `second` | `const void *second` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `second_size` | `size_t second_size` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `out` | `unsigned char *out` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |
| `capacity` | `size_t capacity` | Actual caller-owned output capacity; it is not permission to write past the buffer. |
| `used` | `size_t *used` | Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below. |

## Description

fvm_srecord_write_pair: canonical integers, bounded S-record framing and checksum validation.

### Preserved implementation/header contract

Scatter-input equivalent of write: concatenate two disjoint borrowed byte spans without
an object-sized temporary buffer. Each span and all outputs must not overlap. All size,
checksum and capacity checks precede output; no allocation or native structure serialization.

## Return value

`FVM_SResult`; direct result constants in this body: FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::wire; C89

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
return fvm_srecord_write_pair(tag, payload, size, 0, 0, out, capacity, used);
```

[Complete calling context](../../protocol/srecord.c#L365)

## See also

- [Implementation: protocol/srecord.c:282](../../protocol/srecord.c#L282)
- [Public declaration](../../include/flashvm/srecord.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
