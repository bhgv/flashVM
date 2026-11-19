# styxorderreceivewindow

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxorderreceivewindow.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <unibus/order.h>

int styxorderreceivewindow(StyxOrder *order, unsigned long long window);
```

## Библиотека

Unibus::queues; C99 + pthread

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `order` | `StyxOrder *order` | Нативный механизм очередей тегов; удерживать до завершения параллельных вызовов. |
| `window` | `unsigned long long window` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |

## Описание

styxorderreceivewindow: независимое состояние тегов life.

### Сохранённый контракт implementation/header

EN: Tighten receive displacement before first RX data, 1/0; equal window remains idempotent.
Borrow engine; native peer has agreed this bound. Existing TX counters are untouched.
RU: Сузить окно приёма до первых RX-данных: 1/0, равное окно идемпотентно. TX не меняется.

## Возвращаемое значение

`int`; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::queues; C99 + pthread

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: order/life.c:246](../../order/life.c#L246)
- [Public declaration](../../order/order.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
