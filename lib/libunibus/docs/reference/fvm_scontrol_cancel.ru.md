# fvm_scontrol_cancel

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_scontrol_cancel.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/scontrol.h>

FVM_SResult fvm_scontrol_cancel(FVM_SControlExchange *x);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `x` | `FVM_SControlExchange *x` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |

## Описание

fvm_scontrol_cancel: коррелированное согласование размера/профиля отдельного тега.

### Сохранённый контракт implementation/header

Abandon pending negotiation, preserve active limits and retired IDs. Late ACK cannot commit.
Host must reconcile an uncertain peer state before resuming payload, or start a new session.

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: FVM_S_INVALID, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::wire; C89, no OS/allocator/threads

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: protocol/readable_parts/scontrol-c/part-002.inc:164](../../protocol/readable_parts/scontrol-c/part-002.inc#L164)
- [Public declaration](../../include/flashvm/scontrol.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
