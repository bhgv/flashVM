# fvm_sstream_reset

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_sstream_reset.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/sstream.h>

FVM_SResult fvm_sstream_reset(FVM_SStream *stream);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `stream` | `FVM_SStream *stream` | Decoder для повторного использования; делает прежний view недействительным и сбрасывает latched errors. |

## Описание

fvm_sstream_reset: инкрементальный проверяемый ввод S-записей и время жизни буфера.

### Сохранённый контракт implementation/header

Invalidate an outstanding view and discard prefix/error without wiping caller bytes.
Owner must quiesce users first; this does not resynchronize a transport or cancel remote
work.

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: FVM_S_INVALID, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::wire; C89, no OS/allocator/threads

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
result = fvm_sstream_reset(&stream);
```

[Complete calling context](../../examples/nested.c#L126)

## См. также

- [Implementation: protocol/sstream.c:113](../../protocol/sstream.c#L113)
- [Public declaration](../../include/flashvm/sstream.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
