# styxclientrecord

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxclientrecord.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/client/client.h>

int styxclientrecord(const void *bytes, size_t size, StyxMeta *output, size_t *consumed);
```

## Библиотека

Styx::client; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `bytes` | `const void *bytes` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `size` | `size_t size` | Запрошенная длина в байтах; соответствующий буфер должен её покрывать. |
| `output` | `StyxMeta *output` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `consumed` | `size_t *consumed` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |

## Описание

styxclientrecord: нативные client-операции stat и время жизни.

### Сохранённый контракт implementation/header

EN/RU: Decode one complete original stat record; bytes borrowed,1/0 and consumed.
Reject malformed/truncated records and unsafe basename, no partial output.
Разобрать одну полную stat-запись;1/0 и consumed, без unsafe basename.

## Возвращаемое значение

`int`; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::client; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
int styxclientrecord(const void *bytes, size_t size, StyxMeta *output, size_t *consumed);
```

[Complete calling context](../../client/readable_parts/client-h/part-003.inc#L86)

## См. также

- [Implementation: client/stat.c:27](../../client/stat.c#L27)
- [Public declaration](../../client/client.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
