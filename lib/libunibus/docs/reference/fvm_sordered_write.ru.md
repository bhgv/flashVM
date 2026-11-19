# fvm_sordered_write

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](fvm_sordered_write.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <flashvm/sorder.h>

FVM_SResult fvm_sordered_write(const FVM_SOrdinal *ordinal,
                               unsigned long tag,
                               const void *bytes,
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
| `ordinal` | `const FVM_SOrdinal *ordinal` | Значение/состояние протокола вызывающего; native layout не является wire-представлением. |
| `tag` | `unsigned long tag` | ID этого направления/пространства; внешний0 зарезервирован для служебного трафика. |
| `bytes` | `const void *bytes` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `size` | `size_t size` | Запрошенная длина в байтах; соответствующий буфер должен её покрывать. |
| `output` | `void *output` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `capacity` | `size_t capacity` | Фактическая ёмкость буфера вызывающего; выход за неё недопустим. |
| `used` | `size_t *used` | Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже. |

## Описание

fvm_sordered_write: проверяемые префиксы номеров вокруг неизменённых S-record байт.

### Сохранённый контракт implementation/header

EN/RU: Encode prefix plus one original S-record atomically, bounded by original global body limit.
Nonoverlapping borrowed inputs/output, no heap; output/used unchanged on any refusal.
Атомарно кодировать префикс и одну исходную S-запись; буферы не пересекаются, выделения памяти нет.

## Возвращаемое значение

`FVM_SResult`; прямые константы результата тела: FVM_S_INVALID, FVM_S_LIMIT. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Unibus::wire; C89, no OS/allocator/threads

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
result = fvm_sordered_write(&ordinal, tag, bytes, size, output, capacity, used);
```

[Complete calling context](../../order/send.c#L84)

## См. также

- [Implementation: protocol/order/packet.c:35](../../protocol/order/packet.c#L35)
- [Public declaration](../../include/flashvm/sorder.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
