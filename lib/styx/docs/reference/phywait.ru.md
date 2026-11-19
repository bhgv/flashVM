# phywait

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](phywait.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/phy/phy.h>

int phywait(RemotePhy *phy, unsigned wanted, int timeout, unsigned *ready);
```

## Библиотека

Styx::carrier; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `phy` | `RemotePhy *phy` | Capability физического носителя; передача владения зависит от конструктора/операции. |
| `wanted` | `unsigned wanted` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `timeout` | `int timeout` | Бюджет нативного ожидания в миллисекундах; поведение нуля и верхнего предела задано контрактом. |
| `ready` | `unsigned *ready` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |

## Описание

phywait: реализация remote_phy/phy и нативное состояние.

### Сохранённый контракт implementation/header

EN/RU: Owner waits for requested READ/WRITE; timeout>=-1,0 polls, returns OK/ERROR and ready bits.
No OS descriptor in this contract; driver integrates its own bus/IRQ/event mechanism.
Owner ждёт READ/WRITE, timeout>=-1;0 poll; OK/ERROR и ready bits, без требования OS fd.

## Возвращаемое значение

`int`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::carrier; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
int status = phywait(mux->phy, wanted, timeout, &ready);
```

[Complete calling context](../../client/mux_loop.c#L400)

## См. также

- [Implementation: remote_phy/phy.c:227](../../remote_phy/phy.c#L227)
- [Public declaration](../../remote_phy/phy.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/remote_phy/README.md)
