# styxorderencode

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxorderencode.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <unibus/order.h>

FVM_SResult styxorderencode(
    StyxOrder *order, unsigned long tag, const void *bytes, size_t size, void *output, size_t capacity, size_t *used);
```

## Библиотека

Unibus::queues; C99 + pthread

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `order` | `StyxOrder *order` | Нативный механизм очередей тегов; удерживать до завершения параллельных вызовов. |
| `tag` | `unsigned long tag` | ID этого направления/пространства; внешний0 зарезервирован для служебного трафика. |
| `bytes` | `const void *bytes` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `size` | `size_t size` | Запрошенная длина в байтах; соответствующий буфер должен её покрывать. |
| `output` | `void *output` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `capacity` | `size_t capacity` | Фактическая ёмкость буфера вызывающего; выход за неё недопустим. |
| `used` | `size_t *used` | Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже. |

## Описание

styxorderencode: независимое состояние тегов send.

### Сохранённый контракт implementation/header

EN/RU: Encode one packet into caller-owned disjoint spool, advance TX only on OK; no transport I/O.
Returned packet must be retained/retried as the SAME bytes until sent or connection retired, not re-encoded.
Pending rollover/MAX exhaustion returns CONFLICT for only this tag. Output/used unchanged on failure.
Кодировать пакет в буфер caller; TX меняется только при OK. Для повторной передачи сохранять те же байты, не назначать новый номер.

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: FVM_S_INVALID, FVM_S_LIMIT. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::queues; C99 + pthread

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
CHECK(styxorderencode(order, 7, "zero", 4, bytes, sizeof(bytes), &size) == FVM_S_LIMIT && size == 777);
```

[Complete calling context](../../order/tests/fault.c#L86)

## См. также

- [Implementation: order/send.c:62](../../order/send.c#L62)
- [Public declaration](../../order/order.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
