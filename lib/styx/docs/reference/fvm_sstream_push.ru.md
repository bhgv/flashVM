# fvm_sstream_push

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_sstream_push.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/sstream.h>

FVM_SResult fvm_sstream_push(FVM_SStream *stream, const void *input, size_t size, FVM_SRecord *view, size_t *used);
```

## Библиотека

Styx::wire; C89

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `stream` | `FVM_SStream *stream` | Инициализированный decoder; после готового view нужен reset до нового push. |
| `input` | `const void *input` | Новый заимствованный фрагмент, отдельно от receive buffer/state; NULL только при size=0. |
| `size` | `size_t size` | Доступные байты нового фрагмента, не общий накопленный размер. |
| `view` | `FVM_SRecord *view` | Публикуется только при OK; заимствует receive storage до reset/disposal. |
| `used` | `size_t *used` | Поглощённые байты фрагмента, также при MORE/data error; сохранять только непоглощённый хвост. Неверные аргументы оставляют used прежним. |

## Описание

fvm_sstream_push: инкрементальный проверяемый ввод S-записей и время жизни буфера.

OK выдаёт максимум одну запись. MORE сохраняет поглощённый вход внутри stream. Ошибки удерживаются до reset; push после готового view даёт INVALID и used=0. Нет I/O и блокировки; параллельный доступ к одному decoder требует внешней синхронизации.

### Сохранённый контракт implementation/header

Consume at most one record, stopping before trailing input. Input must not overlap
buffer/state. used is consumed input even on MORE or a latched data error; unchanged on
invalid arguments. view changes only on OK and borrows buffer until reset/disposal.
Preserve/append only bytes not consumed; consumed bytes are already retained. Zero input
returns MORE for an incomplete record. After OK, push returns INVALID with used=0 until
reset; errors are latched with used=0 on retry. Bounds are enforced as soon as framing is
available, before copying an advertised body.

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: FVM_S_INVALID. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::wire; C89

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: protocol/sstream.c:146](../../protocol/sstream.c#L146)
- [Public declaration](../../include/flashvm/sstream.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
