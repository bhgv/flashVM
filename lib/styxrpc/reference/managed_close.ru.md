# styxrpc_managed_close

[English](managed_close.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/managed.h>
void styxrpc_managed_close(StyxRpcManagedCall *call);
```

Завершает fid и вызывает cancel один раз, если задача ещё ждала. Хост вызывает эту функцию при clunk и обрыве. После закрытия заимствованные аргументы недействительны.

C99. Сериализуйте операции одного fid. `StyxRpc::rpc`.

[Пример](../examples/../tests/managed.c) · [Исходник](../src/managed.c)
