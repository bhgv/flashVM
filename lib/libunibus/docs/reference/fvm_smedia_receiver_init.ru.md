# fvm_smedia_receiver_init

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_smedia_receiver_init.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/smedia.h>

FVM_SResult fvm_smedia_receiver_init(
    FVM_SMediaReceiver *out, unsigned long tag, const FVM_SMediaInfo *info, void *buffer, size_t capacity);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `out` | `FVM_SMediaReceiver *out` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `tag` | `unsigned long tag` | ID этого направления/пространства; внешний0 зарезервирован для служебного трафика. |
| `info` | `const FVM_SMediaInfo *info` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `buffer` | `void *buffer` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `capacity` | `size_t capacity` | Фактическая ёмкость буфера вызывающего; выход за неё недопустим. |

## Описание

fvm_smedia_receiver_init: ограниченное framing media fragments и сборка в буфере вызывающего.

### Сохранённый контракт implementation/header

Validate expected identity and caller capacity before replacing native state; buffer bytes
are untouched. Reinitialization explicitly abandons the previous object at a host-chosen
deadline. All descriptors, buffers and slot input/output must be disjoint. No cleanup
allocation exists.

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::wire; C89, no OS/allocator/threads

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: protocol/readable_parts/smedia-c/part-002.inc:142](../../protocol/readable_parts/smedia-c/part-002.inc#L142)
- [Public declaration](../../include/flashvm/smedia.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
