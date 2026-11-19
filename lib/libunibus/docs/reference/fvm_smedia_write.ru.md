# fvm_smedia_write

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_smedia_write.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/smedia.h>

FVM_SResult fvm_smedia_write(unsigned long tag,
                             const FVM_SMediaInfo *info,
                             unsigned long part,
                             const void *data,
                             size_t size,
                             void *out,
                             size_t capacity,
                             size_t *used);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `tag` | `unsigned long tag` | ID этого направления/пространства; внешний0 зарезервирован для служебного трафика. |
| `info` | `const FVM_SMediaInfo *info` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `part` | `unsigned long part` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `data` | `const void *data` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `size` | `size_t size` | Запрошенная длина в байтах; соответствующий буфер должен её покрывать. |
| `out` | `void *out` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `capacity` | `size_t capacity` | Фактическая ёмкость буфера вызывающего; выход за неё недопустим. |
| `used` | `size_t *used` | Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже. |

## Описание

fvm_smedia_write: ограниченное framing media fragments и сборка в буфере вызывающего.

### Сохранённый контракт implementation/header

Encode one exact part to one padded slot. tag must be nonzero Uint32. Atomic output/used on
failure; payload must have precisely the extent implied by total/part. No allocation or I/O.

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

- [Implementation: protocol/readable_parts/smedia-c/part-002.inc:85](../../protocol/readable_parts/smedia-c/part-002.inc#L85)
- [Public declaration](../../include/flashvm/smedia.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
