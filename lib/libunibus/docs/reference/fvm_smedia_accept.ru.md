# fvm_smedia_accept

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_smedia_accept.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/smedia.h>

FVM_SResult fvm_smedia_accept(FVM_SMediaReceiver *r, const void *input, size_t size, int *complete);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `r` | `FVM_SMediaReceiver *r` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `input` | `const void *input` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `size` | `size_t size` | Запрошенная длина в байтах; соответствующий буфер должен её покрывать. |
| `complete` | `int *complete` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |

## Описание

fvm_smedia_accept: ограниченное framing media fragments и сборка в буфере вызывающего.

### Сохранённый контракт implementation/header

Verify before copying; reject mismatched identity CONFLICT without changing
state/buffer/complete. Exact duplicate OK without increasing received; same part with
different valid bytes CONFLICT. Copy only verified new part. complete is true once every part
arrived, not a playback decision. Caller authenticates packet/session before entry; CRC alone
cannot establish trust.

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: FVM_S_CONFLICT, FVM_S_INVALID, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::wire; C89, no OS/allocator/threads

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
result = fvm_smedia_accept(s->receiver, input, size, &done);
```

[Complete calling context](../../protocol/smeasure.c#L301)

## См. также

- [Implementation: protocol/readable_parts/smedia-c/part-001.inc:399](../../protocol/readable_parts/smedia-c/part-001.inc#L399)
- [Public declaration](../../include/flashvm/smedia.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
