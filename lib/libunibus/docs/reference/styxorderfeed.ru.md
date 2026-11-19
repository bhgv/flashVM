# styxorderfeed

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxorderfeed.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <unibus/order.h>

int styxorderfeed(StyxOrder *order, const void *packet, size_t size);
```

## Библиотека

Unibus::queues; C99 + pthread

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `order` | `StyxOrder *order` | Нативный механизм очередей тегов; удерживать до завершения параллельных вызовов. |
| `packet` | `const void *packet` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `size` | `size_t size` | Запрошенная длина в байтах; соответствующий буфер должен её покрывать. |

## Описание

styxorderfeed: независимое состояние тегов receive.

### Сохранённый контракт implementation/header

EN/RU: Validate and copy exactly one complete prefixed packet, return acceptance/duplicate/window/error above.
Caller retains input through return; no direct client callback, gap in one tag never blocks others.
Проверить и скопировать один пакет с префиксом; результаты перечислены выше. Пропуск одного тега не блокирует другие.

## Возвращаемое значение

`int`; прямые константы результата тела: STYX_ORDER_REFUSED. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::queues; C99 + pthread

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
styxorderfeed(receiver, packet, used) == STYX_ORDER_ACCEPTED; /* EN: Invoke styxorderfeed with these explicit arguments. RU: Вызвать styxorderfeed с этими явными аргументами. */
```

[Complete calling context](../../examples/queues.c#L73)

## См. также

- [Implementation: order/receive.c:66](../../order/receive.c#L66)
- [Public declaration](../../order/order.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
