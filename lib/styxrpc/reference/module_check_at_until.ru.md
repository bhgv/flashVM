# styxrpc_module_check_at_until

[English](module_check_at_until.md) · [Объявление](../include/styxrpc/module_client.h) · [Реализация](../src/client_exchange.c)

```c
#include <styxrpc/module_client.h>

StyxRpcStatus styxrpc_module_check_at_until(struct StyxFid *directory,
    const StyxRpcModuleImport *imports, size_t count, uint32_t pid,
    StyxRpcModuleStop stop, void *state);
```

Хостовая цель C `StyxRpc::launch` проверяет контракты в заимствованном
неоткрытом `directory`. Передайте в `imports` от 1 до
`STYXRPC_MAX_EXPORTS` записей и их число в `count`. Запись содержит
имя, кодек, сигнатуру и требуемую схему, если она нужна. Если методу
нужен встречный контекст, укажите действительный ненулевой `pid`.
Поля описаны в [блокирующем API](module_check_at.ru.md).

`STYXRPC_OK` означает совпадение интерфейса, нужных маркеров и
метаданных файлов методов. Методы не запускаются. `STYXRPC_INVALID`
означает неверные аргументы, отказ доступа, отсутствующий или
несовпадающий контракт. `STYXRPC_CANCELLED` означает отмену проверки.

Необязательный `stop(state)` исполняется в потоке вызова. Ненулевой
результат останавливает проверку. NULL оставляет обычное ожидание.
Удерживайте состояние, импорты и каталог до возврата. В
мультиплексированной сессии обход, открытие, чтение или stat отменяется
по тегу. Очистка временных fid может продолжаться после возврата.
Согласуйте закрытие каталога с проверкой.

```c
const StyxRpcModuleImport current = {
    "current", STYXRPC_MODULE_INT, STYXRPC_INT_SIGNATURE, {0}
};
StyxRpcStatus status = styxrpc_module_check_at_until(directory, &current,
    1, 0, stop_requested, task);
```

Возьмите каталог от [живой загрузки](module_live_open_until.ru.md) и
экспорт `current` из [библиотеки теста](../tests/flashvm/live-dis_library.b).
`stop_requested(task)` проверяет флаг остановки вызывающего. После
успеха используйте [проверенный вызов](scalar_invoke_at_until.ru.md).
