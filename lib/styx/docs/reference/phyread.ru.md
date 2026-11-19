# phyread

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](phyread.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/phy/phy.h>

PhyTransfer phyread(RemotePhy *phy, void *bytes, size_t size);
```

## Библиотека

Styx::carrier; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `phy` | `RemotePhy *phy` | Capability физического носителя; передача владения зависит от конструктора/операции. |
| `bytes` | `void *bytes` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `size` | `size_t size` | Запрошенная длина в байтах; соответствующий буфер должен её покрывать. |

## Описание

phyread: реализация remote_phy/phy и нативное состояние.

### Сохранённый контракт implementation/header

EN/RU: Owner-serialized nonblocking read/write, borrowed buffers; validate precise driver results.
Size0 succeeds without a callback; cap hints bound a single driver request.
Owner-serialized read/write, buffers borrowed; size0 без callback, cap limits учитываются.

## Возвращаемое значение

`PhyTransfer`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::carrier; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
PhyTransfer out = phyread(right, NULL, 0);
```

[Complete calling context](../../tests/memory_contract.c#L141)

## См. также

- [Implementation: remote_phy/phy.c:155](../../remote_phy/phy.c#L155)
- [Public declaration](../../remote_phy/phy.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/remote_phy/README.md)
