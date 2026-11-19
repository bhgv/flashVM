# fvm_sbindings_lookup

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_sbindings_lookup.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/sbindings.h>

FVM_SResult fvm_sbindings_lookup(const FVM_SBindings *table, unsigned long tag, FVM_SBinding *out);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `table` | `const FVM_SBindings *table` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `tag` | `unsigned long tag` | ID этого направления/пространства; внешний0 зарезервирован для служебного трафика. |
| `out` | `FVM_SBinding *out` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |

## Описание

fvm_sbindings_lookup: привязки числовых тегов направления к идентичности схемы.

### Сохранённый контракт implementation/header

Copy an association to caller output. UNKNOWN for zero/unbound tags; outputs unchanged on
error. Lookup copies metadata, not a VM/native object; no allocation or implicit permission
grant.

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

- [Implementation: protocol/sbindings.c:247](../../protocol/sbindings.c#L247)
- [Public declaration](../../include/flashvm/sbindings.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
