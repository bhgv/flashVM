# sizeS2M

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](sizeS2M.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/codec.h>

uint sizeS2M(Fcall *f);
```

## Библиотека

Styx::codec; C99, no OS/allocator/threads; alternatively Styx::nine in the hosted build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `f` | `Fcall *f` | Входное сообщение для измерения с допустимыми строками и payload. |

## Описание

Вычислить точный размер стандартного сообщения до кодирования; отклонить неподдерживаемые поля.

Рассчитать ёмкость до convS2M; ноль означает неверное/неподдерживаемое либо переполняющее сообщение, не пустой успешный frame. У стандартного frame есть явные size/type/tag.

### Сохранённый контракт implementation/header

Validate variable fields before sizing or touching the output buffer.

## Возвращаемое значение

`uint`; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::codec; C99, no OS/allocator/threads; alternatively Styx::nine in the hosted build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
size = sizeS2M(f);
```

[Complete calling context](../../vendor/inferno/lib9/readable_parts/convS2M-c/part-001.inc#L335)

## См. также

- [Implementation: vendor/inferno/lib9/readable_parts/convS2M-c/part-001.inc:115](../../vendor/inferno/lib9/readable_parts/convS2M-c/part-001.inc#L115)
- [Public declaration](../../include/styx/fcall.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
