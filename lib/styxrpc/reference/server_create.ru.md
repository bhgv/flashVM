# styxrpc_server_create

[English](server_create.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/serve.h>
StyxRpcServer *styxrpc_server_create(struct RemotePhy *phy, const StyxRpcTree *tree, void *context);
```

Принимает выданный поток и заимствует tree/context до destroy. Корень имеет ID ноль, остальные ID уникальны и устойчивы в соединении. Колбэк stat возвращает имя, родителя, длину в байтах и ревизию каталога. Колбэк enumerate получает каталог и индекс от нуля и возвращает одного потомка либо NULL. При изменении состава или порядка меняйте ревизию. Чтение, запись и проход разрешают биты other режима Unix. Подключённый поток должен быть уже выдан нужному клиенту. Возвращает владеющий указатель либо NULL.

Hosted Linux64. `StyxRpc::serve`.

[Пример](../tests/directory.c) · [Исходник](../src/serve.c)
