# styxrpc_program_unibus

[English](program_unibus.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/program.h>
int styxrpc_program_unibus(StyxRpcPrograms *programs, uint32_t pid, const StyxRpcResource *resource);
```

Регистрирует один двусторонний файл .unibus для PID. Ресурс обычно даёт отдельное состояние на открытие и завершает ожидающую работу при закрытии.

Linux64. Возвращает 1 при успехе, 0 при отказе. Регистрируйте в потоке диспетчера или до его запуска. `StyxRpc::serve`.

[Пример](../examples/disvm/reverse.c) · [Исходник](../src/program.c)

[`styxrpc_program_unibus_revoke`](program_unibus_revoke.ru.md)
