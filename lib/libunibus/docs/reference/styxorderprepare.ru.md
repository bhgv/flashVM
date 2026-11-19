# styxorderprepare

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxorderprepare.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <unibus/order.h>

int styxorderprepare(
    StyxOrder *order, unsigned long tag, unsigned direction, unsigned long long operation, unsigned long long last);
```

## Библиотека

Unibus::queues; C99 + pthread

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `order` | `StyxOrder *order` | Нативный механизм очередей тегов; удерживать до завершения параллельных вызовов. |
| `tag` | `unsigned long tag` | ID этого направления/пространства; внешний0 зарезервирован для служебного трафика. |
| `direction` | `unsigned direction` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `operation` | `unsigned long long operation` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `last` | `unsigned long long last` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |

## Описание

styxorderprepare: независимое состояние тегов rollover.

### Сохранённый контракт implementation/header

EN/RU: Native queued-reset proposal for ONE counter; strictly increasing nonzero local operation ID.
TX boundary must equal last spooled ordinal. RX boundary may wait for its old prefix to arrive/be consumed.
1/0, no wire traffic; host queues service request after old packets and ACKs only when ready returns1.
Подготовить сброс одного счётчика: 1/0. Host сам передаёт служебные request/ACK; остальные теги не затронуты.

## Возвращаемое значение

`int`; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::queues; C99 + pthread

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
CHECK(styxorderprepare(order, 7, STYX_ORDER_RX, 1, 1));
```

[Complete calling context](../../order/tests/fault.c#L93)

## См. также

- [Implementation: order/rollover.c:89](../../order/rollover.c#L89)
- [Public declaration](../../order/order.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
