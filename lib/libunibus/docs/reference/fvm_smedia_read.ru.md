# fvm_smedia_read

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_smedia_read.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/smedia.h>

FVM_SResult fvm_smedia_read(const void *input, size_t size, unsigned long tag, FVM_SMediaPiece *out);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `input` | `const void *input` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `size` | `size_t size` | Запрошенная длина в байтах; соответствующий буфер должен её покрывать. |
| `tag` | `unsigned long tag` | ID этого направления/пространства; внешний0 зарезервирован для служебного трафика. |
| `out` | `FVM_SMediaPiece *out` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |

## Описание

fvm_smedia_read: ограниченное framing media fragments и сборка в буфере вызывающего.

### Сохранённый контракт implementation/header

Verify slot, tag, canonical fields and exact part extent before publishing borrowed piece.
UNKNOWN means another direction-bound tag. Output unchanged on every error.

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

- [Implementation: protocol/readable_parts/smedia-c/part-002.inc:116](../../protocol/readable_parts/smedia-c/part-002.inc#L116)
- [Public declaration](../../include/flashvm/smedia.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
