# fvm_sinteger_write

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](fvm_sinteger_write.ru.md) · [Index](../index.md)

## Syntax

```c
#include <flashvm/srecord.h>

FVM_SResult fvm_sinteger_write(const FVM_SInteger *value, unsigned char *out, size_t capacity, size_t *used);
```

## Library

Styx::wire; C89

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `value` | `const FVM_SInteger *value` | Caller-owned protocol value/state; native layout is never a wire representation. |
| `out` | `unsigned char *out` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |
| `capacity` | `size_t capacity` | Actual caller-owned output capacity; it is not permission to write past the buffer. |
| `used` | `size_t *used` | Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below. |

## Description

fvm_sinteger_write: canonical integers, bounded S-record framing and checksum validation.

### Preserved implementation/header contract

Canonical big-endian signed magnitude. Nonoverlapping outputs required. No allocation/VM.
Write/read value and used only on OK; null input is valid only when size is zero (MORE).
Write needs valid value/output/used. Read needs value/used. Maximum magnitude is 2^64-1.

## Return value

`FVM_SResult`; direct result constants in this body: FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::wire; C89

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
return fvm_sinteger_write(&number, out, FVM_SRECORD_INTEGER_MAX, used);
```

[Complete calling context](../../protocol/srecord.c#L166)

## See also

- [Implementation: protocol/srecord.c:53](../../protocol/srecord.c#L53)
- [Public declaration](../../include/flashvm/srecord.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
