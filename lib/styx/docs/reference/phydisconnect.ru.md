# phydisconnect

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](phydisconnect.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/phy/phy.h>

void phydisconnect(RemotePhy *phy);
```

## Библиотека

Styx::carrier; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `phy` | `RemotePhy *phy` | Capability физического носителя; передача владения зависит от конструктора/операции. |

## Описание

phydisconnect: реализация remote_phy/phy и нативное состояние.

### Сохранённый контракт implementation/header

EN/RU: Owner disconnects once; destroy/free only after every I/O/wake user retires.
Owner disconnect один раз; free после всех I/O/wake users.

## Возвращаемое значение

Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::carrier; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
phydisconnect(mux->phy);
```

[Complete calling context](../../client/mux_loop.c#L432)

## См. также

- [Implementation: remote_phy/phy.c:347](../../remote_phy/phy.c#L347)
- [Public declaration](../../remote_phy/phy.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/remote_phy/README.md)
