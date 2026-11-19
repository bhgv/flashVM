# styxadddir

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxadddir.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>
#include <styxserver.h>

Styxfile *styxadddir(Styxserver *server, Path pqid, Path qid, char *name, int mode, char *owner);
```

## Библиотека

Styx::server; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `server` | `Styxserver *server` | Экземпляр исходного сервера и состояние его dispatcher. |
| `pqid` | `Path pqid` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `qid` | `Path qid` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `name` | `char *name` | Имя компонента либо схемы операции; функция проверяет свои ограничения кодирования/длины. |
| `mode` | `int mode` | Режим операции/доступа; использовать протокольные константы, не суффикс имени. |
| `owner` | `char *owner` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |

## Описание

styxadddir: исходная диспетчеризация сессий/fid и callback-операции namespace.

## Возвращаемое значение

`Styxfile *`; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::server; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
check(styxadddir(&server, Qroot, 1, "nested", 0555, "alice") != nil, "nested dir");
```

[Complete calling context](../../vendor/inferno/tools/libstyx/tests/lifecycle.c#L158)

## См. также

- [Implementation: vendor/inferno/tools/libstyx/readable_parts/styxserver-c/part-005.inc:297](../../vendor/inferno/tools/libstyx/readable_parts/styxserver-c/part-005.inc#L297)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
