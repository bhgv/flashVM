# styxrpc_program_fd

[English](program_fd.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/program.h>
int styxrpc_program_fd(StyxRpcPrograms *programs, uint32_t pid, unsigned fd, unsigned access,
    uint64_t length, const StyxRpcResource *resource);
```

Регистрирует FD 0–31 у существующего PID. Режим, длина в байтах и колбэки описывают один выданный ресурс.

Linux64. Возвращает 1 при успехе, 0 при отказе. Регистрируйте в потоке диспетчера или до его запуска. `StyxRpc::serve`.

[Пример](../examples/disvm/reverse.c) · [Исходник](../src/program.c)
