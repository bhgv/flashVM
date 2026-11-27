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

Необязательный колбэк `stop` синхронно получает заимствованный `state`
в вызывающем потоке. Ненулевой результат останавливает загрузку перед
отправкой, между опросами готовности либо перед выдачей fid. Проверяйте
флаг остановки и возвращайтесь сразу. Удерживайте `state` до возврата
функции. При NULL функция опрашивает готовность обычным способом.

При отмене возвращается `STYXRPC_IO_CANCELLED`. Некорректный запрос
или недоступный управляющий файл дают `STYXRPC_IO_NOT_OPEN`. Отказ
загрузки образа, неверная публикация и истечение ожидания дают
`STYXRPC_IO_FAILED`. Неподтверждённая запись может дать
`STYXRPC_IO_UNCERTAIN`. При любом неуспехе функция сохраняет оба слота
вывода и закрывает временные fid. Уже подтверждённый запрос снимается
при обработке clunk другой стороной. SDK не повторяет запись.

На опрос готовности отведено четыре секунды. Отмена кооперативная.
Начатый запрос Styx использует сроки смонтированной сессии. Вызывайте
функцию в потоке, где можно ждать I/O. Каждый вызов использует свои fid.

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
