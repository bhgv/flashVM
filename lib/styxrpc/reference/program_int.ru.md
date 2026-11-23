# styxrpc_program_int

[English](program_int.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/program.h>
int styxrpc_program_int(StyxRpcPrograms *programs, uint32_t pid, const char *name, const StyxIntVariable *variable);
```

Регистрирует именованный int32. Дескриптор копируется, задаёт отдельные load/store. Нужен хотя бы один из них.

Linux64. Возвращает 1 при успехе, 0 при отказе. Регистрируйте в потоке диспетчера или до его запуска. `StyxRpc::serve`.

[Пример](../examples/disvm/reverse.c) · [Исходник](../src/program.c)
