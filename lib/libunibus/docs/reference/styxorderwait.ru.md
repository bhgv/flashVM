# styxorderwait

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxorderwait.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <unibus/order.h>

int styxorderwait(StyxOrder *order, unsigned long tag, unsigned timeout);
```

## Библиотека

Unibus::queues; C99 + pthread

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `order` | `StyxOrder *order` | Живой native queue engine; удерживать до завершения этого wait и параллельных вызовов. |
| `tag` | `unsigned long tag` | Наблюдается только очередь этого тега направления. |
| `timeout` | `unsigned timeout` | Бюджет по monotonic clock в миллисекундах 0..120000; ноль проверяет текущую готовность. |

## Описание

styxorderwait: независимое состояние тегов inbox.

Ненулевой результат только при готовой записи выбранной очереди. Ноль — timeout, closed-empty, неверные аргументы либо native clock/wait error. Причины нуля не различаются; lifecycle проверяется отдельно. У других тегов независимая готовность.

### Сохранённый контракт implementation/header

EN/RU: Block only selected-tag native consumer under one monotonic budget 0..120000ms, 1 ready/0 otherwise.
Ждать только выбранный тег: 1 — готов, 0 — иначе. Без потребления пакета и остановки брокера.

## Возвращаемое значение

`int`; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::queues; C99 + pthread

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
CHECK(styxorderwait(owner, 7, 3000));
```

[Complete calling context](../../order/tests/queues.c#L25)

## См. также

- [Implementation: order/inbox.c:90](../../order/inbox.c#L90)
- [Public declaration](../../order/order.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
