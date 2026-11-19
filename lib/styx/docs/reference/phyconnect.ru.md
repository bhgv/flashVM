# phyconnect

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](phyconnect.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/phy/phy.h>

int phyconnect(RemotePhy *phy, const void *parameters, size_t size);
```

## Библиотека

Styx::carrier; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `phy` | `RemotePhy *phy` | Capability физического носителя; передача владения зависит от конструктора/операции. |
| `parameters` | `const void *parameters` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `size` | `size_t size` | Запрошенная длина в байтах; соответствующий буфер должен её покрывать. |

## Описание

phyconnect: реализация remote_phy/phy и нативное состояние.

### Сохранённый контракт implementation/header

EN/RU: Owner connects with borrowed driver-native parameters; OK updates local connected state.
Live carrier accepts only empty idempotent connect, not reconfiguration.
Owner connect с borrowed driver-native parameters; OK обновляет connected.

## Возвращаемое значение

`int`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::carrier; hosted native build

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: remote_phy/phy.c:315](../../remote_phy/phy.c#L315)
- [Public declaration](../../remote_phy/phy.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/remote_phy/README.md)
