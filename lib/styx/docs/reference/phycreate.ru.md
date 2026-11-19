# phycreate

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](phycreate.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/phy/phy.h>

RemotePhy *phycreate(const PhyOps *ops, void *state, const PhyInfo *info);
```

## Библиотека

Styx::carrier; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `ops` | `const PhyOps *ops` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `state` | `void *state` | Непрозрачный контекст callback; время жизни/передача заданы создающим API. |
| `info` | `const PhyInfo *info` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |

## Описание

phycreate: реализация remote_phy/phy и нативное состояние.

### Сохранённый контракт implementation/header

EN/RU: Borrow ops/info, adopt state only on owned success; NULL invalid/OOM.
Borrow ops/info; state принят только при success, NULL invalid/OOM.

## Возвращаемое значение

`RemotePhy *`; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::carrier; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
RemotePhy *phy = phycreate(&ops, fd, &info);
```

[Complete calling context](../../remote_phy/drivers/posix/fd.c#L346)

## См. также

- [Implementation: remote_phy/phy.c:55](../../remote_phy/phy.c#L55)
- [Public declaration](../../remote_phy/phy.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/remote_phy/README.md)
