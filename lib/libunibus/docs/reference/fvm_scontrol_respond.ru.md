# fvm_scontrol_respond

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_scontrol_respond.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/scontrol.h>

FVM_SResult fvm_scontrol_respond(FVM_SControlExchange *x, const FVM_SControlMessage *offer, FVM_SControlMessage *out);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `x` | `FVM_SControlExchange *x` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `offer` | `const FVM_SControlMessage *offer` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `out` | `FVM_SControlMessage *out` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |

## Описание

fvm_scontrol_respond: коррелированное согласование размера/профиля отдельного тега.

### Сохранённый контракт implementation/header

Select bounded limits from an authenticated offer and local maxima, store pending selection,
return ACCEPT. Unsupported fixed mode UNKNOWN. No active configuration changes yet.

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: FVM_S_CONFLICT, FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK, FVM_S_UNKNOWN. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::wire; C89, no OS/allocator/threads

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: protocol/readable_parts/scontrol-c/part-002.inc:55](../../protocol/readable_parts/scontrol-c/part-002.inc#L55)
- [Public declaration](../../include/flashvm/scontrol.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
