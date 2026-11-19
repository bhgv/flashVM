# convS2M

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](convS2M.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/codec.h>

uint convS2M(Fcall *f, uchar *ap, uint nap);
```

## Библиотека

Styx::codec; C99, no OS/allocator/threads; alternatively Styx::nine in the hosted build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `f` | `Fcall *f` | Входные поля и заимствованные строки/data; владение остаётся у вызывающего. |
| `ap` | `uchar *ap` | Выходной буфер вызывающего, не перекрывающий входные поля. |
| `nap` | `uint nap` | Доступная выходная ёмкость; sizeS2M сообщает размер целого сообщения. |

## Описание

Кодировать native Fcall в стандартные 9P2000 байты с явным little-endian полей.

f и строки/data остаются у вызывающего; ap имеет ёмкость nap. До записи проверяются тип и размеры counted fields. Ноль означает неподдерживаемые/неверные поля либо недостаточный буфер. Codec не выделяет память, не отправляет запрос и не выполняет операцию файловой системы.

### Сохранённый контракт implementation/header

Serialize only messages whose complete wire size has been validated.

## Возвращаемое значение

`uint`; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::codec; C99, no OS/allocator/threads; alternatively Styx::nine in the hosted build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
uint size = convS2M(&input, bytes, sizeof(bytes)); /* EN: Invoke convS2M with these explicit arguments. RU: Вызвать convS2M с этими явными аргументами. */
```

[Complete calling context](../../examples/codec.c#L33)

## См. также

- [Implementation: vendor/inferno/lib9/readable_parts/convS2M-c/part-001.inc:330](../../vendor/inferno/lib9/readable_parts/convS2M-c/part-001.inc#L330)
- [Public declaration](../../include/styx/fcall.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
