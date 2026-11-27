# styxrpc_cell_set

[English](cell_set.md) · [Объявление](../include/styxrpc/named_cell.h#L59) · [Реализация](../src/named_channel_client.c#L335)

Заменяет текущий кадр через существующий fid ячейки.

## Синтаксис

```c
#include <styxrpc/named_cell.h>

StyxRpcIoOutcome styxrpc_cell_set(StyxRpcVfsFile *lease, const void *frame, size_t size);
```

## Доступность

Хостовый C SDK, цель `StyxRpc::launch`. Нужны подключённый клиент Styx и поддержка общих ячеек на `/.channels` у провайдера. Пример проверен с Linux64 и DisVM.

## Параметры

| Имя | Описание |
| --- | --- |
| `lease` | Открытый fid ячейки на чтение и запись, удерживаемый вызывающим. Функция заимствует его. |
| `frame` | Заимствованный кадр значения с тегом. Доступен для чтения до возврата. |
| `size` | Длина кадра вместе с тегом от 1 до `STYXRPC_NAMED_MAX` байтов. Дополнительные ограничения зависят от тега. |

## Возвращаемое значение

`STYXRPC_IO_OK` подтверждает полную замену. Отсутствующий fid или неверный кадр даёт `STYXRPC_IO_NOT_OPEN`. Определённый отказ или короткий подтверждённый обмен даёт `STYXRPC_IO_FAILED`. При отсутствии положительного подтверждения возможен `STYXRPC_IO_UNCERTAIN`.

## Поведение и время жизни

Согласуйте запросы и закрытие `lease`. Принятое целиком тело становится видимым атомарно. После неопределённого подтверждения отдельное чтение может показать текущее значение, но за это время другой участник может выполнить запись. Закройте fid с оборванным кадром.

Вызов может ждать I/O по срокам смонтированной сессии.

## Пример

Передайте действующую аренду чтения и записи. Пример записывает полный кадр Boolean со значением false.

```c
const unsigned char disabled[] = {STYXRPC_NAMED_BOOLEAN, 0};
StyxRpcIoOutcome result = styxrpc_cell_set(lease, disabled, sizeof(disabled));
```

После отказа может потребоваться закрыть `lease`. При восстановлении используйте отдельную действующую аренду.

## См. также

[Общие ячейки](named_cell.ru.md) · [Формат значения](cell_value.ru.md) · [styxrpc_cell_write_until](cell_write_until.ru.md) · [styxrpc_vfs_close](vfs_close.ru.md) · [Пошаговый пример C и Limbo](../examples/shared-cell/README.ru.md)
