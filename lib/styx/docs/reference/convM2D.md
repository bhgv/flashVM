# convM2D

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](convM2D.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/codec.h>

uint convM2D(uchar *buf, uint nbuf, Dir *d, char *strs);
```

## Library

Styx::codec; C99, no OS/allocator/threads; alternatively Styx::nine in the hosted build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `buf` | `uchar *buf` | Input counted stat bytes; no native Dir layout is accepted. |
| `nbuf` | `uint nbuf` | Accessible input extent in bytes. |
| `d` | `Dir *d` | Caller-owned decoded scalar fields and string pointers. |
| `strs` | `char *strs` | Caller-owned decoded-string storage, or NULL for empty string fields. nbuf bytes suffice for a valid record. |

## Description

Decode a validated standard Dir record and copy its strings into caller-supplied storage when provided.

The input record is bounded by nbuf. Supply caller-owned strs storage large enough for all decoded strings and their terminators; using nbuf bytes is sufficient for a validated record. NULL strs requests scalar metadata with empty string fields. No strings are allocated by the codec. The returned extent is one decoded record; zero rejects invalid/truncated input.

### Preserved implementation/header contract

Decode only the declared first record, leaving outputs untouched on bad input.

## Return value

`uint`; direct result constants in this body: 0. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::codec; C99, no OS/allocator/threads; alternatively Styx::nine in the hosted build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
check(convM2D((uchar *)out.data, out.count, &d, strings) == out.count, "partial record");
```

[Complete calling context](../../vendor/inferno/tools/libstyx/tests/directories.c#L239)

## See also

- [Implementation: vendor/inferno/lib9/convM2D.c:72](../../vendor/inferno/lib9/convM2D.c#L72)
- [Public declaration](../../include/styx/fcall.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
