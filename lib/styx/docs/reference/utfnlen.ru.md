# utfnlen

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](utfnlen.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <lib9.h>

int utfnlen(char *s, long m);
```

## Библиотека

Styx::nine; hosted Linux64 support

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `s` | `char *s` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `m` | `long m` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |

## Описание

utfnlen: исходная POSIX-поддержка lib9 utfnlen.

### Сохранённый контракт implementation/header

Count complete runes without forming an end pointer beyond the actual string.

## Возвращаемое значение

`int`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::nine; hosted Linux64 support

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
return _fmtcpy(f, s, utfnlen(s, i), i);
```

[Complete calling context](../../vendor/inferno/lib9/readable_parts/dofmt-c/part-002.inc#L68)

## См. также

- [Implementation: vendor/inferno/lib9/utfnlen.c:22](../../vendor/inferno/lib9/utfnlen.c#L22)
- [Public declaration](../../vendor/inferno/Linux/386/include/lib9.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
