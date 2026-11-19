# fvm_sorder_control_read

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_sorder_control_read.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/sordctl.h>

FVM_SResult fvm_sorder_control_read(const void *input, size_t size, FVM_SOrderControl *message);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `input` | `const void *input` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `size` | `size_t size` | Запрошенная длина в байтах; соответствующий буфер должен её покрывать. |
| `message` | `FVM_SOrderControl *message` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |

## Описание

fvm_sorder_control_read: именованные order-service offer/accept/renewal записи.

### Сохранённый контракт implementation/header

EN: Read exactly one named service body, copying scalars only on OK. UNKNOWN for other identities.
Known malformed messages return INVALID/checksum; trailing records/integers are invalid.
RU: Прочитать тело именованной службы; скаляры меняются лишь при OK, другая служба — UNKNOWN.
Ошибка известной службы и лишние данные отвергаются. Input заимствован до возврата.

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: FVM_S_INVALID, FVM_S_OK, FVM_S_UNKNOWN. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::wire; C89, no OS/allocator/threads

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: protocol/order/control.c:150](../../protocol/order/control.c#L150)
- [Public declaration](../../include/flashvm/sordctl.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
