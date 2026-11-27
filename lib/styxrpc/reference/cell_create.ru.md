# styxrpc_cell_create

[English](cell_create.md) · [Объявление](../include/styxrpc/named_cell.h#L46) · [Реализация](../src/named_channel_client.c#L275)

Создаёт ячейку, выдаёт первую аренду и жетон для передачи.

## Синтаксис

```c
#include <styxrpc/named_cell.h>

StyxRpcVfsFile *styxrpc_cell_create(struct StyxClient *session,
    const void *frame, size_t size, void *token);
```

## Доступность

Хостовый C SDK, цель `StyxRpc::launch`. Нужны подключённый клиент Styx и поддержка общих ячеек на `/.channels` у провайдера. Пример проверен с Linux64 и DisVM.

## Параметры

| Имя | Описание |
| --- | --- |
| `session` | Заимствованный подключённый клиент службы `/.channels` выдавшей стороны. Удерживайте его до возврата. |
| `frame` | Заимствованный кадр значения с тегом. Доступен для чтения до возврата. |
| `size` | Длина кадра вместе с тегом от 1 до `STYXRPC_NAMED_MAX` байтов. Дополнительные ограничения зависят от тега. |
| `token` | Записываемый буфер на `STYXRPC_CELL_TOKEN` байтов (24). Заполняется при успехе. |

## Возвращаемое значение

При успехе возвращает собственный fid и заполняет `token`. NULL означает неверный аргумент, отказ или незавершённый обмен. При отказе буфер жетона сохраняет содержимое.

## Поведение и время жизни

Выдавшая сторона проверяет целый кадр и назначает новый жетон. После последнего локального обращения вызовите `styxrpc_vfs_close`. При потере ответа сервер может временно удерживать созданную ячейку. Вызов с отказом закрывает временный fid.

Вызов может ждать I/O по срокам смонтированной сессии.

## Пример

Подключённую сессию передаёт приложение. В примере буфер жетона локален, а созданный fid освобождается.

```c
unsigned char token[STYXRPC_CELL_TOKEN];
const unsigned char initial[] = {STYXRPC_NAMED_BOOLEAN, 1};
StyxRpcVfsFile *owner = styxrpc_cell_create(session, initial, sizeof(initial), token);
if (owner)
{
    styxrpc_vfs_close(owner, 0);
}
```

При обмене удерживайте `owner`, пока получатель не присоединится.

## См. также

[Общие ячейки](named_cell.ru.md) · [Формат значения](cell_value.ru.md) · [styxrpc_cell_create_until](cell_create_until.ru.md) · [styxrpc_vfs_close](vfs_close.ru.md) · [Пошаговый пример C и Limbo](../examples/shared-cell/README.ru.md)
