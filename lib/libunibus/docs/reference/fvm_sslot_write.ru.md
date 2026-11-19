# fvm_sslot_write

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_sslot_write.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/sslot.h>

FVM_SResult
fvm_sslot_write(unsigned long tag, const void *payload, size_t size, void *output, size_t capacity, size_t *used);
```

## Библиотека

Unibus::wire; C89, no OS/allocator/threads

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `tag` | `unsigned long tag` | ID этого направления/пространства; внешний0 зарезервирован для служебного трафика. |
| `payload` | `const void *payload` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `size` | `size_t size` | Запрошенная длина в байтах; соответствующий буфер должен её покрывать. |
| `output` | `void *output` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `capacity` | `size_t capacity` | Фактическая ёмкость буфера вызывающего; выход за неё недопустим. |
| `used` | `size_t *used` | Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже. |

## Описание

fvm_sslot_write: проверяемая геометрия fixed slots и размеры compact records.

### Сохранённый контракт implementation/header

One complete record followed by zero padding; input/output/state must not overlap.
No allocation, transport, VM or tag binding. Writer publishes exactly SSLOT_BYTES on OK.
The slot profile/size must be agreed over a trusted control channel, not inferred from
damage.

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::wire; C89, no OS/allocator/threads

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: protocol/sslot.c:81](../../protocol/sslot.c#L81)
- [Public declaration](../../include/flashvm/sslot.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/srecords/README.md)
