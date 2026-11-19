# walk_packet_write

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](walk_packet_write.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/walk/packet.h>

void *walk_packet_write(
    unsigned kind, unsigned long tag, const WalkEntity *root, const void *frame, size_t frame_size, size_t *length);
```

## Библиотека

Styx::walk; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `kind` | `unsigned kind` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `tag` | `unsigned long tag` | ID этого направления/пространства; внешний0 зарезервирован для служебного трафика. |
| `root` | `const WalkEntity *root` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `frame` | `const void *frame` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `frame_size` | `size_t frame_size` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `length` | `size_t *length` | Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием. |

## Описание

walk_packet_write: проверяемые префиксы номеров вокруг неизменённых S-record байт.

### Сохранённый контракт implementation/header

EN/RU: Build owned transport envelope/NULL with little-endian size and two checked S-records; caller frees result.
Request captures this caller's active history now; reply advertises borrowed nonzero root and empty history.
Frame/tag/root inputs borrowed through return; no provider callbacks/VM/code execution. length written on success only.
Создать owned envelope/NULL с size и двумя checked S-records; caller освобождает. Request снимает active history,
reply объявляет nonzero root и пустую history; inputs borrowed до return, length лишь при success.

## Возвращаемое значение

`void *`; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::walk; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
walk_packet_write(WALK_PACKET_REQUEST, mux->walk_tag, NULL, beat->bytes, sizeof(beat->bytes), &encoded);
```

[Complete calling context](../../client/heartbeat.c#L123)

## См. также

- [Implementation: walk/packet.c:83](../../walk/packet.c#L83)
- [Public declaration](../../walk/packet.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
