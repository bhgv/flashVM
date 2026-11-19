# styxclientmountphy

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxclientmountphy.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/client/client.h>

StyxClient *
styxclientmountphy(RemotePhy *phy, unsigned msize, const char *user, const char *name, unsigned timeout, unsigned limit);
```

## Библиотека

Styx::client; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `phy` | `RemotePhy *phy` | Capability физического носителя; передача владения зависит от конструктора/операции. |
| `msize` | `unsigned msize` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `user` | `const char *user` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `name` | `const char *name` | Имя компонента либо схемы операции; функция проверяет свои ограничения кодирования/длины. |
| `timeout` | `unsigned timeout` | Бюджет нативного ожидания в миллисекундах; поведение нуля и верхнего предела задано контрактом. |
| `limit` | `unsigned limit` | Явная локальная квота; диапазон и единицы wire/body указаны в описании. |

## Описание

styxclientmountphy: нативные client-операции phy и время жизни.

### Сохранённый контракт implementation/header

EN/RU: Consume owned connected reliable ordered byte-stream PHY on every outcome; owned client/NULL.
Uses the same native multiplex/heartbeat/cancel engine as mountfd, with no fd requirement.
Ordinary single 9P session: limit bounds outstanding RPCs, never enables P9MX/reverse sessions.
Driver callbacks remain native-only; stream adaptations must precede this admission boundary.
Принять connected reliable ordered stream PHY всегда; свой client/NULL, общий native engine,
без fd requirement; limit ограничивает RPC одной сессии, не включает P9MX.
Packet/reliability adaptations должны предшествовать этому уровню.

## Возвращаемое значение

`StyxClient *`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::client; hosted native build

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: client/phy.c:32](../../client/phy.c#L32)
- [Public declaration](../../client/client.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
