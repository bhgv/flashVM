# getwd

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](getwd.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

char *getwd(char *buf, int size);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `buf` | `char *buf` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `size` | `int size` | Запрошенная длина в байтах; соответствующий буфер должен её покрывать. |

## Описание

getwd: исходная POSIX-поддержка lib9 getwd-Nt.

## Возвращаемое значение

`char *`; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: vendor/inferno/lib9/getwd-Nt.c:20](../../vendor/inferno/lib9/getwd-Nt.c#L20)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
