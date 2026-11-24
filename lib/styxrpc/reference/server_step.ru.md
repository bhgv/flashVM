# styxrpc_server_step

[English](server_step.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/serve.h>
int styxrpc_server_step(StyxRpcServer *server);
```

Ожидает событие и обрабатывает его в потоке владельца. Возвращает 1 для продолжения, 0 при обрыве или ошибке. Колбэки выполняются в том же потоке. Не вызывайте step параллельно или рекурсивно на одном сервере. Смещения каталогов измеряются в байтах кодированных записей. При изменении состава откройте каталог заново и начните чтение с нуля.

Hosted Linux64. `StyxRpc::serve`.

[Пример](../tests/directory.c) · [Исходник](../src/serve.c)

[Обслуживать запросы до ближайшего срока](server_step_for.ru.md).
