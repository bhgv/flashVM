# fvm_sstream_reset

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_sstream_reset.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/sstream.h>

FVM_SResult fvm_sstream_reset(FVM_SStream *stream);
```

## Библиотека

Styx::wire; C89

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

Styx::wire; C89

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: protocol/sstream.c:113](../../protocol/sstream.c#L113)
- [Public declaration](../../include/flashvm/sstream.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
