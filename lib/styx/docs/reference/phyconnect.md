# phyconnect

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](phyconnect.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/phy/phy.h>

int phyconnect(RemotePhy *phy, const void *parameters, size_t size);
```

## Library

Styx::carrier; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `phy` | `RemotePhy *phy` | Physical-carrier capability; ownership transfer depends on the constructor/operation. |
| `parameters` | `const void *parameters` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `size` | `size_t size` | Requested byte extent; the relevant buffer must cover it. |

## Description

phyconnect: remote_phy/phy implementation and native state.

### Preserved implementation/header contract

EN/RU: Owner connects with borrowed driver-native parameters; OK updates local connected state.
Live carrier accepts only empty idempotent connect, not reconfiguration.
Owner connect с borrowed driver-native parameters; OK обновляет connected.

## Return value

`int`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::carrier; hosted native build

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: remote_phy/phy.c:315](../../remote_phy/phy.c#L315)
- [Public declaration](../../remote_phy/phy.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/remote_phy/README.md)
