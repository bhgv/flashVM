# styxrpc_module_live_open_until

[English](module_live_open_until.md) · [Объявление](../include/styxrpc/module_client.h) ·
[Реализация](../src/live_module_client.c)

```c
#include <styxrpc/module_client.h>

typedef int (*StyxRpcModuleStop)(void *state);
StyxRpcIoOutcome styxrpc_module_live_open_until(struct StyxClient *session,
    const char *path, struct StyxFid **directory, struct StyxFid **lease,
    StyxRpcModuleStop stop, void *state);
```

Используйте хостовую цель `StyxRpc::launch`. Заимствуйте подключённую
`session` и канонический путь к `.dis` относительно корня, как у
[`styxrpc_module_live_open`](module_live_open.ru.md). Передайте разные
слоты для точного неоткрытого каталога и аренды запроса. При
`STYXRPC_IO_OK` функция передаёт владение обоими fid. Удерживайте аренду
до завершения вызовов и использования каналов результата. Освободите
оба fid через `styxclientclunk`, когда их владельцы закончат работу.

Необязательный `stop(state)` исполняется в потоке вызова. Ненулевой
результат останавливает открытие службы, отправку пути, ожидание
готовности или получение поколения. Удерживайте `state` до возврата.
NULL оставляет обычный блокирующий обмен и опрос готовности.

`STYXRPC_IO_CANCELLED` означает отмену. Неверный запрос или недоступный
файл управления даёт `STYXRPC_IO_NOT_OPEN`. Отказ загрузки, неверная
публикация или истечение ожидания даёт `STYXRPC_IO_FAILED`.
Запись без подтверждения может вернуть `STYXRPC_IO_UNCERTAIN`.
При отказе оба выхода сохраняются. Временный запрос снимается, когда
clunk достигает провайдера. Это может произойти после возврата.
SDK не повторяет запись.

Бюджет опроса готовности составляет четыре секунды. Мультиплексированная
сессия отменяет ожидающий обход, stat, открытие, чтение или запись по тегу.
Подтверждение flush и очистка используют сроки сессии. Поток должен
допускать ожидание I/O. Отдельные вызовы используют отдельные fid.

```c
struct StyxFid *directory = NULL;
struct StyxFid *lease = NULL;
StyxRpcIoOutcome outcome = styxrpc_module_live_open_until(session,
    "lib/counter.dis", &directory, &lease, stop_requested, task);
if (outcome == STYXRPC_IO_OK) {
    /* Получить и вызвать точные экспорты до освобождения обоих fid. */
    styxclientclunk(directory);
    styxclientclunk(lease);
}
```

Здесь `stop_requested(task)` проверяет флаг отмены вызывающего. См.
[обнаружение экспортов](module_exports_at.ru.md) и
[живую загрузку в FlashVM](../../../flashVM/doc/STYX_RPC_LIVE_DIS.ru.md).
