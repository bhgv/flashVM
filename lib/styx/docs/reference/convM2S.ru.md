# convM2S

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](convM2S.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/codec.h>

uint convM2S(uchar *ap, uint nap, Fcall *f);
```

## Библиотека

Styx::codec; C99, no OS/allocator/threads; alternatively Styx::nine in the hosted build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `ap` | `uchar *ap` | Изменяемое входное сообщение; строки/data результата заимствуют эту память. |
| `nap` | `uint nap` | Доступные входные байты, включая четырёхбайтовую длину сообщения. |
| `f` | `Fcall *f` | Приёмник Fcall. При возврате нуля частично заполненные поля отбрасываются. |

## Описание

Декодировать исходное 9P2000-сообщение с проверкой counted fields внутри объявленной длины.

ap — изменяемый буфер сообщения длиной nap. Counted strings сдвигаются на месте для NUL-терминаторов. Строки/data Fcall заимствуют ap до конца обработки. Ноль означает неполный/неверный вход: частичным полям доверять нельзя. Успех возвращает длину сообщения, не sizeof(Fcall).

### Сохранённый контракт implementation/header

no syntactic checks.
three causes for error:
 1. message size field is incorrect
 2. input buffer too short for its own data (counts too long, etc.)
 3. too many names or qids
gqid() and gstring() return nil if they would reach beyond buffer.
main switch statement checks range and also can fall through
to test at end of routine.

## Возвращаемое значение

`uint`; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::codec; C99, no OS/allocator/threads; alternatively Styx::nine in the hosted build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
check(nreply > 0 && convM2S(reply, nreply, &out) == nreply, "decode");
```

[Complete calling context](../../vendor/inferno/tools/libstyx/tests/directories.c#L182)

## См. также

- [Implementation: vendor/inferno/lib9/convM2S.c:105](../../vendor/inferno/lib9/convM2S.c#L105)
- [Public declaration](../../include/styx/fcall.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
