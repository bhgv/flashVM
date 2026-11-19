# styxorderready

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxorderready.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <unibus/order.h>

int styxorderready(StyxOrder *order, unsigned long tag, unsigned direction, unsigned long long operation);
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

## Описание

styxorderready: независимое состояние тегов rollover.

### Сохранённый контракт implementation/header

EN/RU: Copy readiness of this exact pending rollover,1/0; RX requires consumed boundary and empty held/ready queues.
Готовность точно выбранного сброса: 1/0. RX требует выдачи boundary и пустых очередей held/ready.

## Возвращаемое значение

`int`; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::queues; C99 + pthread

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
CHECK(!styxorderready(order, 7, STYX_ORDER_RX, 1));
```

[Complete calling context](../../order/tests/rollover.c#L76)

## См. также

- [Implementation: order/rollover.c:137](../../order/rollover.c#L137)
- [Public declaration](../../order/order.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
