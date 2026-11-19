# setbinmode

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](setbinmode.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

void setbinmode(void);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| — | void | Параметров нет. |

## Описание

setbinmode: исходная POSIX-поддержка lib9 setbinmode-Nt.

### Сохранённый контракт implementation/header

	Extensions for Inferno to basic libc.h

## Возвращаемое значение

Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: vendor/inferno/lib9/setbinmode-Nt.c:16](../../vendor/inferno/lib9/setbinmode-Nt.c#L16)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
