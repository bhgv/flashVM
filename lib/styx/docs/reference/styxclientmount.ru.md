# styxclientmount

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxclientmount.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/client/client.h>

StyxClient *styxclientmount(const StyxTransport *transport, unsigned msize, const char *user, const char *name);
```

## Библиотека

Styx::client; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `transport` | `const StyxTransport *transport` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `msize` | `unsigned msize` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `user` | `const char *user` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `name` | `const char *name` | Имя компонента либо схемы операции; функция проверяет свои ограничения кодирования/длины. |

## Описание

styxclientmount: нативные client-операции session и время жизни.

### Сохранённый контракт implementation/header

EN/RU: Consume a valid transport on every outcome; negotiate256..65536 bytes and
attach NOFID with copied host-supplied identity hints. Invalid callback struct
is not adopted. Caller serializes creation; returned connection is owned/NULL.
Принять valid transport всегда; negotiate+attach, свой client/NULL.

## Возвращаемое значение

`StyxClient *`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::client; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
StyxClient *styxclientmount(const StyxTransport *transport, unsigned msize, const char *user, const char *name);
```

[Complete calling context](../../client/readable_parts/client-h/part-001.inc#L53)

## См. также

- [Implementation: client/readable_parts/session-c/part-002.inc:120](../../client/readable_parts/session-c/part-002.inc#L120)
- [Public declaration](../../client/client.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
