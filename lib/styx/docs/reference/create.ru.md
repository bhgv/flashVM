# create

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](create.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

int create(char *f, int mode, int perm);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `f` | `char *f` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `mode` | `int mode` | Режим операции/доступа; использовать протокольные константы, не суффикс имени. |
| `perm` | `int perm` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |

## Описание

create: исходная POSIX-поддержка lib9 create.

### Сохранённый контракт implementation/header

of loved one & descendants

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

- [Implementation: vendor/inferno/lib9/create.c:27](../../vendor/inferno/lib9/create.c#L27)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
