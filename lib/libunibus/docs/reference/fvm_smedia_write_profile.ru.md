# fvm_smedia_write_profile

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_smedia_write_profile.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/smedia.h>

FVM_SResult fvm_smedia_write_profile(const FVM_SMediaProfile *profile,
                                     unsigned long tag,
                                     const FVM_SMediaInfo *info,
                                     unsigned long part,
                                     const void *data,
                                     size_t size,
                                     void *output,
                                     size_t capacity,
                                     size_t *used);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `profile` | `const FVM_SMediaProfile *profile` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `tag` | `unsigned long tag` | ID этого направления/пространства; внешний0 зарезервирован для служебного трафика. |
| `info` | `const FVM_SMediaInfo *info` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `part` | `unsigned long part` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `data` | `const void *data` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `size` | `size_t size` | Запрошенная длина в байтах; соответствующий буфер должен её покрывать. |
| `output` | `void *output` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `capacity` | `size_t capacity` | Фактическая ёмкость буфера вызывающего; выход за неё недопустим. |
| `used` | `size_t *used` | Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже. |

## Описание

fvm_smedia_write_profile: ограниченное framing media fragments и сборка в буфере вызывающего.

### Сохранённый контракт implementation/header

Explicit-profile variants use already agreed geometry, preserve the default wrappers and
require no object-sized scratch allocation. Total remains bounded by MAX_BYTES and PARTS.
Profile changes require a quiesced/new object; a data packet cannot change the profile.

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::wire; C89, no OS/allocator/threads

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
return fvm_smedia_write_profile(&media_default, tag, info, part, data, size, out, capacity, used);
```

[Complete calling context](../../protocol/readable_parts/smedia-c/part-002.inc#L94)

## См. также

- [Implementation: protocol/readable_parts/smedia-c/part-001.inc:185](../../protocol/readable_parts/smedia-c/part-001.inc#L185)
- [Public declaration](../../include/flashvm/smedia.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
