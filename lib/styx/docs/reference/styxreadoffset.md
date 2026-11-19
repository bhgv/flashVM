# styxreadoffset

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxreadoffset.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>
#include <styxserver.h>

vlong styxreadoffset(StyxRead *request);
```

## Library

Styx::server; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `request` | `StyxRead *request` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |

## Description

styxreadoffset: libstyx/read implementation and native state.

### Preserved implementation/header contract

Borrow pending client/fid aux and exact capacity/offset only on native socket owner.
Client/aux become nil on retirement; zero limit then distinguishes inactivity.
Заимствовать client/fid aux, capacity/offset на owner; после retirement nil/zero.

## Return value

`vlong`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::server; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
vlong styxreadoffset(StyxRead *);
```

[Complete calling context](../../vendor/inferno/include/readable_parts/styxserver-h/part-002.inc#L131)

## See also

- [Implementation: vendor/inferno/tools/libstyx/read.c:203](../../vendor/inferno/tools/libstyx/read.c#L203)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
