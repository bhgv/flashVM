# styxordercreate

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxordercreate.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <unibus/order.h>

StyxOrder *styxordercreate(unsigned limit, unsigned long long window);
```

## Библиотека

Unibus::queues; C99 + pthread

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `limit` | `unsigned limit` | Явная локальная квота; диапазон и единицы wire/body указаны в описании. |
| `window` | `unsigned long long window` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |

## Описание

styxordercreate: независимое состояние тегов life.

### Сохранённый контракт implementation/header

EN/RU: Owned engine/NULL, no worker or transport; body limit 1..S_BODY_MAX, reorder window 1..2^63-1.
Host feeds validated lanes on native broker threads; clients wait for their own ready tag.
Вернуть собственный engine либо NULL, без worker/transport; limit ограничивает тело записи, window — перестановку номеров, а не длину очереди клиента.

## Возвращаемое значение

`StyxOrder *`; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::queues; C99 + pthread

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
StyxOrder *sender = styxordercreate(256, 16); /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
```

[Complete calling context](../../examples/queues.c#L57)

## См. также

- [Implementation: order/life.c:63](../../order/life.c#L63)
- [Public declaration](../../order/order.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
