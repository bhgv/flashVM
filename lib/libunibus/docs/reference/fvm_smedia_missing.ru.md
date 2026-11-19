# fvm_smedia_missing

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_smedia_missing.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/smedia.h>

FVM_SResult fvm_smedia_missing(
    const FVM_SMediaReceiver *r, unsigned long start, unsigned long *parts, size_t capacity, size_t *count);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `r` | `const FVM_SMediaReceiver *r` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `start` | `unsigned long start` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `parts` | `unsigned long *parts` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `capacity` | `size_t capacity` | Фактическая ёмкость буфера вызывающего; выход за неё недопустим. |
| `count` | `size_t *count` | Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием. |

## Описание

fvm_smedia_missing: ограниченное framing media fragments и сборка в буфере вызывающего.

### Сохранённый контракт implementation/header

Enumerate absent part indices >=start, at most capacity, in ascending order. Continue with
the last returned index+1. FILE reports holes; LIVE always returns count zero. No timer,
network request, retry storm or transfer acknowledgement is generated. Invalid args leave
output/count unchanged. A host bounds/rate-limits requests and retains sender data through
file completion.

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

- [Implementation: protocol/readable_parts/smedia-c/part-002.inc:30](../../protocol/readable_parts/smedia-c/part-002.inc#L30)
- [Public declaration](../../include/flashvm/smedia.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
