# phywrite

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](phywrite.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/phy/phy.h>

PhyTransfer phywrite(RemotePhy *phy, const void *bytes, size_t size);
```

## Библиотека

Styx::carrier; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `phy` | `RemotePhy *phy` | Capability физического носителя; передача владения зависит от конструктора/операции. |
| `bytes` | `const void *bytes` | Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием. |
| `size` | `size_t size` | Запрошенная длина в байтах; соответствующий буфер должен её покрывать. |

## Описание

phywrite: реализация remote_phy/phy и нативное состояние.

### Сохранённый контракт implementation/header

EN/RU: Owner-serialized precise write of borrowed bytes/size; no replay or splitting on failure.
Owner-serialized precise write borrowed bytes/size; без replay/splitting при failure.

## Возвращаемое значение

`PhyTransfer`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::carrier; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
phywrite(mux->phy, (beat->wire ? beat->wire : beat->bytes) + beat->sent, beat->length - beat->sent);
```

[Complete calling context](../../client/heartbeat.c#L175)

## См. также

- [Implementation: remote_phy/phy.c:190](../../remote_phy/phy.c#L190)
- [Public declaration](../../remote_phy/phy.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/remote_phy/README.md)
