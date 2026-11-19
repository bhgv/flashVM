# phyfree

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](phyfree.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/phy/phy.h>

void phyfree(RemotePhy *phy);
```

## Библиотека

Styx::carrier; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `phy` | `RemotePhy *phy` | Capability физического носителя; передача владения зависит от конструктора/операции. |

## Описание

phyfree: реализация remote_phy/phy и нативное состояние.

### Сохранённый контракт implementation/header

EN/RU: Consume owned carrier after all I/O/wake callers retire; NULL harmless.
Поглотить owned carrier после всех I/O/wake callers; NULL harmless.

## Возвращаемое значение

Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::carrier; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
phyfree(client_phy);
```

[Complete calling context](../../examples/memory_phy/main.c#L78)

## См. также

- [Implementation: remote_phy/phy.c:370](../../remote_phy/phy.c#L370)
- [Public declaration](../../remote_phy/phy.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/remote_phy/README.md)
