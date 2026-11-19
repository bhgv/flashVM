# styxorderreached

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxorderreached.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <unibus/order.h>

int styxorderreached(StyxOrder *order, unsigned long tag, unsigned long long boundary);
```

## Библиотека

Unibus::queues; C99 + pthread

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `order` | `StyxOrder *order` | Нативный механизм очередей тегов; удерживать до завершения параллельных вызовов. |
| `tag` | `unsigned long tag` | ID этого направления/пространства; внешний0 зарезервирован для служебного трафика. |
| `boundary` | `unsigned long long boundary` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |

## Описание

styxorderreached: независимое состояние тегов inbox.

### Сохранённый контракт implementation/header

EN/RU: Non-consuming old-prefix checkpoint,1/0; packet merely held/ready is not considered consumed.
Native host uses this per-tag watermark before applying assignment requests across lanes.
Проверить отметку выданного номера тега: 1/0. Записи в held/ready ещё не выданы; host использует отметку при переназначении.

## Возвращаемое значение

`int`; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::queues; C99 + pthread

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
CHECK(styxorderreached(order, 7, 0) && !styxorderreached(order, 7, 1));
```

[Complete calling context](../../order/tests/queues.c#L86)

## См. также

- [Implementation: order/inbox.c:142](../../order/inbox.c#L142)
- [Public declaration](../../order/order.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
