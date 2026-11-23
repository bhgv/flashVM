# styxrpc_program_variable

[English](program_variable.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/program.h>
int styxrpc_program_variable(StyxRpcPrograms *programs, uint32_t pid, const char *name, unsigned access,
    uint64_t length, const StyxRpcResource *resource);
```

Регистрирует переменную под .var с кодировкой приложения. Для совместимого int32 используйте program_int.

Linux64. Возвращает 1 при успехе, 0 при отказе. Регистрируйте в потоке диспетчера или до его запуска. `StyxRpc::serve`.

[Пример](../examples/disvm/reverse.c) · [Исходник](../src/program.c)
