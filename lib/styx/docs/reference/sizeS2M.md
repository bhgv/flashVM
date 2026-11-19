# sizeS2M

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](sizeS2M.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/codec.h>

uint sizeS2M(Fcall *f);
```

## Library

Styx::codec; C99, no OS/allocator/threads; alternatively Styx::nine in the hosted build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `f` | `Fcall *f` | Input message to measure, with valid referenced strings and payload. |

## Description

Calculate the exact standard message size before encoding; reject unsupported or unrepresentable fields.

Calculate capacity before convS2M; zero is an invalid/unsupported or overflowing message, not a successful empty frame. Every standard frame includes its explicit size/type/tag header.

### Preserved implementation/header contract

Validate variable fields before sizing or touching the output buffer.

## Return value

`uint`; direct result constants in this body: 0. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::codec; C99, no OS/allocator/threads; alternatively Styx::nine in the hosted build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
size = sizeS2M(f);
```

[Complete calling context](../../vendor/inferno/lib9/readable_parts/convS2M-c/part-001.inc#L335)

## See also

- [Implementation: vendor/inferno/lib9/readable_parts/convS2M-c/part-001.inc:115](../../vendor/inferno/lib9/readable_parts/convS2M-c/part-001.inc#L115)
- [Public declaration](../../include/styx/fcall.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
