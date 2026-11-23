# styxrpc_program_add

[English](program_add.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/program.h>
int styxrpc_program_add(StyxRpcPrograms *programs, uint32_t pid);
```

Создаёт пять узлов процесса для ненулевого PID. Дубли и нехватка памяти оставляют дерево прежним.

Linux64. Возвращает 1 при успехе, 0 при отказе. Регистрируйте в потоке диспетчера или до его запуска. `StyxRpc::serve`.

[Пример](../examples/disvm/reverse.c) · [Исходник](../src/program.c)
