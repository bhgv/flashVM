# fvm_sslot_write_sized

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_sslot_write_sized.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/sslot.h>

FVM_SResult fvm_sslot_write_sized(
    unsigned long tag, const void *payload, size_t size, size_t slot_bytes, void *output, size_t capacity, size_t *used);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `tag` | `unsigned long tag` | ID этого направления/пространства; внешний0 зарезервирован для служебного трафика. |
| `payload` | `const void *payload` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `size` | `size_t size` | Запрошенная длина в байтах; соответствующий буфер должен её покрывать. |
| `slot_bytes` | `size_t slot_bytes` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `output` | `void *output` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `capacity` | `size_t capacity` | Фактическая ёмкость буфера вызывающего; выход за неё недопустим. |
| `used` | `size_t *used` | Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже. |

## Описание

fvm_sslot_write_sized: проверяемая геометрия fixed slots и размеры compact records.

### Сохранённый контракт implementation/header

Explicit-size variants use an already agreed slot extent, MIN_BYTES..MAX_BYTES. They have
the same ownership/atomicity rules as the default 1024-byte wrappers, and allocate nothing.
A size_t unable to represent the negotiated extent must reject it at host negotiation.

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::wire; C89, no OS/allocator/threads

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
return fvm_sslot_write_sized(tag, payload, size, FVM_SSLOT_BYTES, output, capacity, used);
```

[Complete calling context](../../protocol/sslot.c#L83)

## См. также

- [Implementation: protocol/sslot.c:34](../../protocol/sslot.c#L34)
- [Public declaration](../../include/flashvm/sslot.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
