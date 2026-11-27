# styxrpc_module_exports_scoped_at_until

[English](module_exports_scoped_at_until.md) · [Объявление](../include/styxrpc/module_client.h) · [Реализация](../src/client_exports.inc)

```c
#include <styxrpc/module_client.h>

StyxRpcStatus styxrpc_module_exports_scoped_at_until(struct StyxFid *directory,
    uint32_t pid, StyxRpcModuleExports *out, uint32_t *scoped,
    StyxRpcModuleStop stop, void *state);
```

Хостовая цель C `StyxRpc::launch` обнаруживает методы в заимствованном
неоткрытом `directory`, проверяя весь интерфейс, нужные маркеры и метаданные
файлов методов. Если методу нужен встречный контекст, передайте
действительный ненулевой `pid` вызывающего.

После `STYXRPC_OK` структура `out` владеет именами и контрактами.
Сохраняйте её адрес, так как имена ссылаются внутрь структуры.
Если `scoped` задан, бит `i` показывает, требуется ли
`out->methods[i]` встречный контекст. NULL отключает выдачу маски.
`STYXRPC_INVALID` означает отказ, несовместимый или пустой интерфейс
либо неверные аргументы. `STYXRPC_CANCELLED` означает отмену. При обоих
отказах выходы сохраняют прежние значения.

Необязательный `stop(state)` исполняется в потоке вызова. Ненулевой
результат останавливает работу. Удерживайте состояние и каталог до возврата.
NULL оставляет обычное ожидание. В мультиплексированной сессии ожидающий
обмен отменяется по тегу. Очистка временных fid может продолжиться после
возврата. Для параллельных вызовов нужны отдельные выходы. Согласуйте их
с закрытием каталога.

```c
StyxRpcModuleExports exports;
uint32_t scoped;
StyxRpcStatus status = styxrpc_module_exports_scoped_at_until(directory,
    pid, &exports, &scoped, stop_requested, task);
```

Передайте [захваченный каталог](module_capture_until.ru.md).
`stop_requested(task)` проверяет флаг остановки вызывающего.
После успеха выберите кодек по найденным контрактам или передайте их
в [проверку импортов](module_check_at_until.ru.md).
