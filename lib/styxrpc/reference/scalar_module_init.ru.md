# styxrpc_scalar_module_init

[English](scalar_module_init.md) · [Заголовок и дерево](../include/styxrpc/scalar_module.h) · [Реализация](../src/scalar_module.c)

Подключите `styxrpc/scalar_module.h` и цель `StyxRpc::serve` хостового SDK.

```c
StyxRpcStatus styxrpc_scalar_module_init(StyxRpcScalarModule *module, const char *name,
    const StyxRpcExport *exports, size_t count);
```

Подготовьте выделенный приложением `module` для публикации как
`mounted/<name>`. Требуется непустой компонент пути до 255 байтов и от одного
до восьми разных скалярных описателей. В описателях указаны имена, компилированные
сигнатуры, колбэки и их контексты. Модуль владеет закодированным `.interface`
и заимствует имена и описатели. `STYXRPC_OK` означает готовность к публикации.
При `STYXRPC_INVALID` результат такой инициализации публиковать нельзя.

Передайте модуль как context и `&styxrpc_scalar_module_tree` как дерево в
[styxrpc_server_create](server_create.ru.md). Диспетчер выделяет состояние на
каждый открытый fid. Функция исполняется синхронно в потоке диспетчера один раз
на полный принятый запрос. Clunk и обрыв освобождают состояние вызова.
Модуль и заимствованные описатели должны оставаться живыми и неизменными
до уничтожения сервера. См. [скалярные вызовы](scalar_invoke.ru.md).
