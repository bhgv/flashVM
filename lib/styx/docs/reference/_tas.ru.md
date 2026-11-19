# _tas

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](_tas.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

int _tas(int *la);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `la` | `int *la` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |

## Описание

_tas: исходная POSIX-поддержка lib9 lock-Nt-386.

### Сохранённый контракт implementation/header

synchronization

## Возвращаемое значение

`int`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
return _tas(&l->val) == 0;
```

[Complete calling context](../../vendor/inferno/lib9/lock.c#L62)

## См. также

- [Implementation: vendor/inferno/lib9/lock-Nt-386.c:17](../../vendor/inferno/lib9/lock-Nt-386.c#L17)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
