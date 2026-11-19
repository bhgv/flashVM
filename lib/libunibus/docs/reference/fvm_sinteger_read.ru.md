# fvm_sinteger_read

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_sinteger_read.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/srecord.h>

FVM_SResult fvm_sinteger_read(const unsigned char *in, size_t size, FVM_SInteger *value, size_t *used);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `in` | `const unsigned char *in` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `size` | `size_t size` | Запрошенная длина в байтах; соответствующий буфер должен её покрывать. |
| `value` | `FVM_SInteger *value` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `used` | `size_t *used` | Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже. |

## Описание

fvm_sinteger_read: канонические целые, framing S-записей и проверка checksum.

### Сохранённый контракт implementation/header

Canonical big-endian signed magnitude. Nonoverlapping outputs required. No allocation/VM.
Write/read value and used only on OK; null input is valid only when size is zero (MORE).
Write needs valid value/output/used. Read needs value/used. Maximum magnitude is 2^64-1.

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: FVM_S_INVALID, FVM_S_MORE, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::wire; C89, no OS/allocator/threads

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
FVM_SResult result = fvm_sinteger_read(*cursor, *left, &number, &used);
```

[Complete calling context](../../protocol/sbindings.c#L30)

## См. также

- [Implementation: protocol/srecord.c:101](../../protocol/srecord.c#L101)
- [Public declaration](../../include/flashvm/srecord.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
