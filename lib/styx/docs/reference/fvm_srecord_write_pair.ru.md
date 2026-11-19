# fvm_srecord_write_pair

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_srecord_write_pair.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/srecord.h>

FVM_SResult fvm_srecord_write_pair(unsigned long tag,
                                   const void *first,
                                   size_t first_size,
                                   const void *second,
                                   size_t second_size,
                                   unsigned char *out,
                                   size_t capacity,
                                   size_t *used);
```

## Библиотека

Styx::wire; C89

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `tag` | `unsigned long tag` | ID этого направления/пространства; внешний0 зарезервирован для служебного трафика. |
| `first` | `const void *first` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `first_size` | `size_t first_size` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `second` | `const void *second` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `second_size` | `size_t second_size` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `out` | `unsigned char *out` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `capacity` | `size_t capacity` | Фактическая ёмкость буфера вызывающего; выход за неё недопустим. |
| `used` | `size_t *used` | Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже. |

## Описание

fvm_srecord_write_pair: канонические целые, framing S-записей и проверка checksum.

### Сохранённый контракт implementation/header

Scatter-input equivalent of write: concatenate two disjoint borrowed byte spans without
an object-sized temporary buffer. Each span and all outputs must not overlap. All size,
checksum and capacity checks precede output; no allocation or native structure serialization.

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::wire; C89

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
return fvm_srecord_write_pair(tag, payload, size, 0, 0, out, capacity, used);
```

[Complete calling context](../../protocol/srecord.c#L365)

## См. также

- [Implementation: protocol/srecord.c:282](../../protocol/srecord.c#L282)
- [Public declaration](../../include/flashvm/srecord.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
