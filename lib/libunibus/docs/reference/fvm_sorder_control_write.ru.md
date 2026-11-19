# fvm_sorder_control_write

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_sorder_control_write.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/sordctl.h>

FVM_SResult fvm_sorder_control_write(const FVM_SOrderControl *message, void *output, size_t capacity, size_t *used);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `message` | `const FVM_SOrderControl *message` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `output` | `void *output` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `capacity` | `size_t capacity` | Фактическая ёмкость буфера вызывающего; выход за неё недопустим. |
| `used` | `size_t *used` | Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже. |

## Описание

fvm_sorder_control_write: именованные order-service offer/accept/renewal записи.

### Сохранённый контракт implementation/header

EN: Encode two complete nested S-records into disjoint output; no outer tag-zero wrapper.
Identity record tag0, command record tag1..6, six canonical unsigned integers in fixed order.
Borrow message/output; outputs unchanged on any failure, no allocation, C89 ABI.
RU: Кодировать две вложенные S-записи без внешнего tag0; шесть канонических чисел.
Буферы не пересекаются; отказ не меняет output/used, выделения памяти нет.

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::wire; C89, no OS/allocator/threads

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: protocol/order/control.c:92](../../protocol/order/control.c#L92)
- [Public declaration](../../include/flashvm/sordctl.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
