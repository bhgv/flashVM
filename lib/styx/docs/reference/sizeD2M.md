# sizeD2M

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](sizeD2M.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/codec.h>

uint sizeD2M(Dir *d);
```

## Library

Styx::codec; C99, no OS/allocator/threads; alternatively Styx::nine in the hosted build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `d` | `Dir *d` | Input directory metadata; strings must be valid NUL-terminated C strings. |

## Description

Calculate the complete standard stat-record size, including its leading size field.

The result includes the leading two-byte count. Each string has its own uint16 count; oversized strings/combined record lengths return zero. Directory offsets advance by this complete encoded size, never by the number of children.

### Preserved implementation/header contract

Report zero when the stat size cannot be represented on the wire.

## Return value

`uint`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::codec; C99, no OS/allocator/threads; alternatively Styx::nine in the hosted build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
uint required = sizeD2M(&info);
```

[Complete calling context](../../examples/server.c#L270)

## See also

- [Implementation: vendor/inferno/lib9/convD2M.c:69](../../vendor/inferno/lib9/convD2M.c#L69)
- [Public declaration](../../include/styx/fcall.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
