# fvm_sbindings_init

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_sbindings_init.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/sbindings.h>

FVM_SResult fvm_sbindings_init(FVM_SBindings *table, FVM_SBinding *slots, size_t capacity);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `table` | `FVM_SBindings *table` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `slots` | `FVM_SBinding *slots` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `capacity` | `size_t capacity` | Фактическая ёмкость буфера вызывающего; выход за неё недопустим. |

## Описание

fvm_sbindings_init: привязки числовых тегов направления к идентичности схемы.

### Сохранённый контракт implementation/header

Initialize an empty table without modifying slots; caller synchronizes all shared use.
Capacity zero/null slots is valid. Reinitializing invalidates prior session associations.

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: FVM_S_INVALID, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::wire; C89, no OS/allocator/threads

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
fvm_sbindings_init(&bindings, slots, 2) == FVM_S_OK; /* EN: Invoke fvm_sbindings_init with these explicit arguments. RU: Вызвать fvm_sbindings_init с этими явными аргументами. */
```

[Complete calling context](../../examples/bridge/main.c#L177)

## См. также

- [Implementation: protocol/sbindings.c:217](../../protocol/sbindings.c#L217)
- [Public declaration](../../include/flashvm/sbindings.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
