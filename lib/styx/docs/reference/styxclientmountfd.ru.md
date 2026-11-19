# styxclientmountfd

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxclientmountfd.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/client/client.h>

StyxClient *
styxclientmountfd(int fd, unsigned msize, const char *user, const char *name, unsigned timeout, unsigned limit);
```

## Библиотека

Styx::client; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `fd` | `int fd` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `msize` | `unsigned msize` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `user` | `const char *user` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `name` | `const char *name` | Имя компонента либо схемы операции; функция проверяет свои ограничения кодирования/длины. |
| `timeout` | `unsigned timeout` | Бюджет нативного ожидания в миллисекундах; поведение нуля и верхнего предела задано контрактом. |
| `limit` | `unsigned limit` | Явная локальная квота; диапазон и единицы wire/body указаны в описании. |

## Описание

styxclientmountfd: нативные client-операции fd и время жизни.

### Сохранённый контракт implementation/header

EN/RU: Consume connected POSIX stream fd always; independent native owner supports
up to limit1..128 outstanding RPCs. timeout1..120000ms per request, then at most
one extra timeout interval for flush. No transport authentication is implied.
Принять connected POSIX fd; отдельный native owner, bounded RPC/flush.

## Возвращаемое значение

`StyxClient *`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::client; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
StyxClient *client = styxclientmountfd(descriptor, 8192, "demo", "", 5000, 8);
```

[Complete calling context](../../examples/client.c#L51)

## См. также

- [Implementation: client/fd.c:34](../../client/fd.c#L34)
- [Public declaration](../../client/client.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
