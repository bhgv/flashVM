# styxorderstart

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxorderstart.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <unibus/order.h>

int styxorderstart(StyxOrder *order, unsigned long tag, unsigned direction, unsigned long long first);
```

## Библиотека

Unibus::queues; C99 + pthread

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `order` | `StyxOrder *order` | Нативный механизм очередей тегов; удерживать до завершения параллельных вызовов. |
| `tag` | `unsigned long tag` | ID этого направления/пространства; внешний0 зарезервирован для служебного трафика. |
| `direction` | `unsigned direction` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `first` | `unsigned long long first` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |

## Описание

styxorderstart: независимое состояние тегов life.

### Сохранённый контракт implementation/header

EN/RU: Native-only seed an unused directional counter before peer traffic,1/0; default starts at0.
Existing/used direction cannot be overwritten. Caller agrees initial value with peer; no inferred wire command.
Задать first для неиспользованного tag/direction: 1/0; по умолчанию ноль. Начальное значение явно согласуется с другой стороной.

## Возвращаемое значение

`int`; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::queues; C99 + pthread

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
CHECK(styxorderstart(order, 7, STYX_ORDER_RX, 0));
```

[Complete calling context](../../order/tests/fault.c#L88)

## См. также

- [Implementation: order/life.c:119](../../order/life.c#L119)
- [Public declaration](../../order/order.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
