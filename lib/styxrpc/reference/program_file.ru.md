# styxrpc_program_file

[English](program_file.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/program.h>
int styxrpc_program_file(StyxRpcPrograms *programs, uint32_t pid, const char *name, unsigned access,
    uint64_t length, const StyxRpcResource *resource);
```

Регистрирует файл в .vfs. Режим открывает ровно указанные операции чтения и записи.

Linux64. Возвращает 1 при успехе, 0 при отказе. Регистрируйте в потоке диспетчера или до его запуска. `StyxRpc::serve`.

[Пример](../examples/disvm/reverse.c) · [Исходник](../src/program.c)
