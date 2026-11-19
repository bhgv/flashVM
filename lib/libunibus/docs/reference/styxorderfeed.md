# styxorderfeed

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxorderfeed.ru.md) · [Index](../index.md)

## Syntax

```c
#include <unibus/order.h>

int styxorderfeed(StyxOrder *order, const void *packet, size_t size);
```

## Library

Unibus::queues; C99 + pthread

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `order` | `StyxOrder *order` | Native per-tag queue engine; keep it alive until all concurrent calls retire. |
| `packet` | `const void *packet` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `size` | `size_t size` | Requested byte extent; the relevant buffer must cover it. |

## Description

styxorderfeed: independent per-tag receive state.

### Preserved implementation/header contract

EN/RU: Validate and copy exactly one complete prefixed packet, return acceptance/duplicate/window/error above.
Caller retains input through return; no direct client callback, gap in one tag never blocks others.
Проверить и скопировать один пакет с префиксом; результаты перечислены выше. Пропуск одного тега не блокирует другие.

## Return value

`int`; direct result constants in this body: STYX_ORDER_REFUSED. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Unibus::queues; C99 + pthread

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
styxorderfeed(receiver, packet, used) == STYX_ORDER_ACCEPTED; /* EN: Invoke styxorderfeed with these explicit arguments. RU: Вызвать styxorderfeed с этими явными аргументами. */
```

[Complete calling context](../../examples/queues.c#L73)

## See also

- [Implementation: order/receive.c:66](../../order/receive.c#L66)
- [Public declaration](../../order/order.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
