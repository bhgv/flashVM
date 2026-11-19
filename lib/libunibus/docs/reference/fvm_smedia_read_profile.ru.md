# fvm_smedia_read_profile

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_smedia_read_profile.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/smedia.h>

FVM_SResult fvm_smedia_read_profile(
    const FVM_SMediaProfile *profile, const void *input, size_t size, unsigned long tag, FVM_SMediaPiece *out);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `profile` | `const FVM_SMediaProfile *profile` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `input` | `const void *input` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `size` | `size_t size` | Запрошенная длина в байтах; соответствующий буфер должен её покрывать. |
| `tag` | `unsigned long tag` | ID этого направления/пространства; внешний0 зарезервирован для служебного трафика. |
| `out` | `FVM_SMediaPiece *out` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |

## Описание

fvm_smedia_read_profile: ограниченное framing media fragments и сборка в буфере вызывающего.

### Сохранённый контракт implementation/header

Decode without accepting tag declarations from a lossy/untrusted media datagram.

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: FVM_S_INVALID, FVM_S_OK, FVM_S_UNKNOWN. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::wire; C89, no OS/allocator/threads

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
result = fvm_smedia_read_profile(&r->profile, input, size, r->tag, &piece);
```

[Complete calling context](../../protocol/readable_parts/smedia-c/part-001.inc#L409)

## См. также

- [Implementation: protocol/readable_parts/smedia-c/part-001.inc:248](../../protocol/readable_parts/smedia-c/part-001.inc#L248)
- [Public declaration](../../include/flashvm/smedia.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
