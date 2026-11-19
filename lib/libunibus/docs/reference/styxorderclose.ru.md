# styxorderclose

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxorderclose.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <unibus/order.h>

void styxorderclose(StyxOrder *order);
```

## Библиотека

Unibus::queues; C99 + pthread

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `order` | `StyxOrder *order` | Нативный механизм очередей тегов; удерживать до завершения параллельных вызовов. |

## Описание

styxorderclose: независимое состояние тегов life.

### Сохранённый контракт implementation/header

EN/RU: Close connection owner, wake all native waits, keep ready records individually drainable.
Закрыть соединение и разбудить ожидающих; готовые записи остаются доступны для поштучного чтения.

## Возвращаемое значение

Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::queues; C99 + pthread

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
styxorderclose(context.receiver);
```

[Complete calling context](../../examples/queues.c#L133)

## См. также

- [Implementation: order/life.c:159](../../order/life.c#L159)
- [Public declaration](../../order/order.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
