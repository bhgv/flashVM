# dirfstat

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](dirfstat.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

Dir *dirfstat(int fd);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `fd` | `int fd` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |

## Описание

dirfstat: исходная POSIX-поддержка lib9 dirstat-Nt.

### Сохранённый контракт implementation/header

last modifier name

## Возвращаемое значение

`Dir *`; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: vendor/inferno/lib9/dirstat-Nt.c:81](../../vendor/inferno/lib9/dirstat-Nt.c#L81)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
