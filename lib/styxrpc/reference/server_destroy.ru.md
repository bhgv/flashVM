# styxrpc_server_destroy

[English](server_destroy.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/serve.h>
void styxrpc_server_destroy(StyxRpcServer *server);
```

Освобождает сервер и закрывает все состояния открытых fid в потоке владельца, в том числе после обрыва. NULL допустим. Состояние приложения должно жить до возврата.

Hosted Linux64. `StyxRpc::serve`.

[Пример](../tests/directory.c) · [Исходник](../src/serve.c)
