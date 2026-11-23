# styxrpc_launch_named_close

[English](launch_named_close.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/launch.h>
void styxrpc_launch_named_close(struct StyxFid **request);
```

Поглощает fid и очищает указатель. Допускает NULL и уже очищенный указатель. Clunk запрашивает отмену ожидающего запуска. Функция не выдаёт квитанцию об отмене или исполнении. Согласуйте закрытие и чтение статуса одного fid.

Hosted Linux64. `StyxRpc::launch`.

[Пример](../examples/disvm/named.c) · [Исходник](../src/launch.c)
