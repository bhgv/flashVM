# phycreate

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](phycreate.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/phy/phy.h>

RemotePhy *phycreate(const PhyOps *ops, void *state, const PhyInfo *info);
```

## Library

Styx::carrier; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `ops` | `const PhyOps *ops` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `state` | `void *state` | Opaque callback context; its lifetime/transfer is specified by the creating API. |
| `info` | `const PhyInfo *info` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |

## Description

phycreate: remote_phy/phy implementation and native state.

### Preserved implementation/header contract

EN/RU: Borrow ops/info, adopt state only on owned success; NULL invalid/OOM.
Borrow ops/info; state принят только при success, NULL invalid/OOM.

## Return value

`RemotePhy *`; direct result constants in this body: NULL. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::carrier; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
RemotePhy *first = phycreate(&ops, &link->ends[0], &info);
```

[Complete calling context](../../examples/memory_phy/memory.c#L90)

## See also

- [Implementation: remote_phy/phy.c:55](../../remote_phy/phy.c#L55)
- [Public declaration](../../remote_phy/phy.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/remote_phy/README.md)
