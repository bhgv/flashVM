# dirfwstat

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](dirfwstat.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

int dirfwstat(int fd, Dir *d);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `fd` | `int fd` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `d` | `Dir *d` | Нативные metadata/ссылки на строки; кодировать поля, не передавать layout структуры. |

## Описание

dirfwstat: исходная POSIX-поддержка lib9 dirwstat.

### Сохранённый контракт implementation/header

last modifier name

## Возвращаемое значение

`int`; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: vendor/inferno/lib9/dirwstat.c:21](../../vendor/inferno/lib9/dirwstat.c#L21)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
