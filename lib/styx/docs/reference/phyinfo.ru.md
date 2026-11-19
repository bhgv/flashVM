# phyinfo

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](phyinfo.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/phy/phy.h>

int phyinfo(RemotePhy *phy, PhyInfo *out);
```

## Библиотека

Styx::carrier; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `phy` | `RemotePhy *phy` | Capability физического носителя; передача владения зависит от конструктора/операции. |
| `out` | `PhyInfo *out` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |

## Описание

phyinfo: реализация remote_phy/phy и нативное состояние.

### Сохранённый контракт implementation/header

EN/RU: Copy local capabilities/lifecycle;1/0, owner-serialized, no driver/transport I/O.
Копировать local info;1/0, owner-serialized, без driver I/O.

## Возвращаемое значение

`int`; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::carrier; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
int bidir = phyinfo(phy, &info) && (info.capabilities & PHY_BIDIR_SESSION);
```

[Complete calling context](../../client/phy.c#L35)

## См. также

- [Implementation: remote_phy/phy.c:97](../../remote_phy/phy.c#L97)
- [Public declaration](../../remote_phy/phy.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/remote_phy/README.md)
