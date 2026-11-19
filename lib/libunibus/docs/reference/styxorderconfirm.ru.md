# styxorderconfirm

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxorderconfirm.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <unibus/order.h>

int styxorderconfirm(
    StyxOrder *order, unsigned long tag, unsigned direction, unsigned long long operation, int accepted);
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
| `accepted` | `int accepted` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |

## Описание

styxorderconfirm: независимое состояние тегов rollover.

### Сохранённый контракт implementation/header

EN/RU: Apply matching accepted ACK(1)/explicit NACK(0),1/0; exact most-recent repeats idempotent.
ACK resets only this directional counter to0. No automatic wrap; wrong/not-ready confirmation changes nothing.
Native host authenticates session/operation and retires old lane deliveries before accepting rollover.
Точное ACK сбрасывает лишь этот tag/direction в ноль: 1/0. Ошибочное либо преждевременное подтверждение ничего не меняет.

## Возвращаемое значение

`int`; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::queues; C99 + pthread

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
CHECK(!styxorderconfirm(order, 7, STYX_ORDER_RX, 1, 1));
```

[Complete calling context](../../order/tests/fault.c#L96)

## См. также

- [Implementation: order/rollover.c:172](../../order/rollover.c#L172)
- [Public declaration](../../order/order.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
