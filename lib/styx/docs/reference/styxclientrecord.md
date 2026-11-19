# styxclientrecord

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxclientrecord.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/client/client.h>

int styxclientrecord(const void *bytes, size_t size, StyxMeta *output, size_t *consumed);
```

## Library

Styx::client; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `bytes` | `const void *bytes` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |
| `size` | `size_t size` | Requested byte extent; the relevant buffer must cover it. |
| `output` | `StyxMeta *output` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |
| `consumed` | `size_t *consumed` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |

## Description

styxclientrecord: native client stat operations and lifetime.

### Preserved implementation/header contract

EN/RU: Decode one complete original stat record; bytes borrowed,1/0 and consumed.
Reject malformed/truncated records and unsafe basename, no partial output.
Разобрать одну полную stat-запись;1/0 и consumed, без unsafe basename.

## Return value

`int`; direct result constants in this body: 0. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::client; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
int styxclientrecord(const void *bytes, size_t size, StyxMeta *output, size_t *consumed);
```

[Complete calling context](../../client/readable_parts/client-h/part-003.inc#L86)

## See also

- [Implementation: client/stat.c:27](../../client/stat.c#L27)
- [Public declaration](../../client/client.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
