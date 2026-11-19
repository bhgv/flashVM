# _assert

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](_assert.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>

void _assert(char *message);
```

## Library

Styx::nine; hosted Linux64 support

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `message` | `char *message` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |

## Description

_assert: posix/fatal implementation and native state.

### Preserved implementation/header contract

EN/RU: Report an upstream invariant and terminate; no allocation or formatter recursion.
Сообщить upstream invariant и завершить; без allocation/formatter recursion.

## Return value

No C return value; output/state/ownership effects are specified in Description.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::nine; hosted Linux64 support

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: ports/posix/fatal.c:22](../../ports/posix/fatal.c#L22)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
