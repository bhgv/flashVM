# physelect

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](physelect.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/phy/phy.h>

RemotePhy *physelect(const PhyDriver *drivers, size_t count, const char *name, const void *parameters, size_t size);
```

## Library

Styx::carrier; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `drivers` | `const PhyDriver *drivers` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `count` | `size_t count` | Extent/count used by the selected conversion; byte/rune/record units follow its signature and description. |
| `name` | `const char *name` | Component or schema name in this operation; the function validates its own encoding/length limits. |
| `parameters` | `const void *parameters` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `size` | `size_t size` | Requested byte extent; the relevant buffer must cover it. |

## Description

physelect: remote_phy/select implementation and native state.

### Preserved implementation/header contract

EN/RU: Select exactly one matching driver from borrowed table; NULL absent/ambiguous/invalid.
Driver-specific native parameters borrowed until its open returns; no fallback to another carrier.
Выбрать один driver из borrowed table; NULL absent/ambiguous/invalid, без fallback.

## Return value

`RemotePhy *`; direct result constants in this body: NULL. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::carrier; hosted native build

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: remote_phy/select.c:30](../../remote_phy/select.c#L30)
- [Public declaration](../../remote_phy/phy.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/remote_phy/README.md)
