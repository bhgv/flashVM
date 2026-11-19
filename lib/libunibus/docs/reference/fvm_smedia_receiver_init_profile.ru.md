# fvm_smedia_receiver_init_profile

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_smedia_receiver_init_profile.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/smedia.h>

FVM_SResult fvm_smedia_receiver_init_profile(FVM_SMediaReceiver *out,
                                             const FVM_SMediaProfile *profile,
                                             unsigned long tag,
                                             const FVM_SMediaInfo *info,
                                             void *buffer,
                                             size_t capacity);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `out` | `FVM_SMediaReceiver *out` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `profile` | `const FVM_SMediaProfile *profile` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `tag` | `unsigned long tag` | ID этого направления/пространства; внешний0 зарезервирован для служебного трафика. |
| `info` | `const FVM_SMediaInfo *info` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `buffer` | `void *buffer` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `capacity` | `size_t capacity` | Фактическая ёмкость буфера вызывающего; выход за неё недопустим. |

## Описание

fvm_smedia_receiver_init_profile: ограниченное framing media fragments и сборка в буфере вызывающего.

### Сохранённый контракт implementation/header

Replace identity/bitmap only after all caller-owned buffer and capacity checks pass.

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::wire; C89, no OS/allocator/threads

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
return fvm_smedia_receiver_init_profile(out, &media_default, tag, info, buffer, capacity);
```

[Complete calling context](../../protocol/readable_parts/smedia-c/part-002.inc#L145)

## См. также

- [Implementation: protocol/readable_parts/smedia-c/part-001.inc:331](../../protocol/readable_parts/smedia-c/part-001.inc#L331)
- [Public declaration](../../include/flashvm/smedia.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
