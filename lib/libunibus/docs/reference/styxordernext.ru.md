# styxordernext

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxordernext.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <unibus/order.h>

int styxordernext(
    StyxOrder *order, unsigned long tag, unsigned long long *ordinal, void *bytes, size_t capacity, size_t *size);
```

## Библиотека

Unibus::queues; C99 + pthread

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `order` | `StyxOrder *order` | Нативный механизм очередей тегов; удерживать до завершения параллельных вызовов. |
| `tag` | `unsigned long tag` | ID этого направления/пространства; внешний0 зарезервирован для служебного трафика. |
| `ordinal` | `unsigned long long *ordinal` | Выходной номер поглощённой записи; без поглощения не меняется. |
| `bytes` | `void *bytes` | Выходной буфер вызывающего либо NULL при capacity=0. |
| `capacity` | `size_t capacity` | Выходная ёмкость; SMALL сохраняет запись в очереди для повторного чтения. |
| `size` | `size_t *size` | Требуемые/фактические байты payload при наличии записи, также при SMALL. |

## Описание

styxordernext: независимое состояние тегов inbox.

RECORD поглощает ровно одну готовую запись; SMALL сообщает требуемый размер без удаления. EMPTY — нет готовой записи открытого engine; END — закрыт/неверен без доступной записи. После close готовые записи ещё можно дочитать. Вызов не ждёт появления данных.

### Сохранённый контракт implementation/header

EN/RU: Copy one ready selected-tag record/SMALL/EMPTY/END; successful pop advances consumed checkpoint.
One consumer per tag, output/ordinal unchanged except successful copy; SMALL changes only size.
Выдать одну готовую запись выбранного тега и отметить выданный номер; SMALL меняет только size, не потребляя запись.

## Возвращаемое значение

`int`; прямые константы результата тела: STYX_ORDER_END. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::queues; C99 + pthread

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
CHECK(styxordernext(order, 7, &ordinal, &tiny, 1, &size) == STYX_ORDER_SMALL);
```

[Complete calling context](../../order/tests/queues.c#L89)

## См. также

- [Implementation: order/inbox.c:32](../../order/inbox.c#L32)
- [Public declaration](../../order/order.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
