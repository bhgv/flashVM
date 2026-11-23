# styxrpc_programs_init

[English](programs_init.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/program.h>
int styxrpc_programs_init(StyxRpcPrograms *programs, StyxRpcRegistration *storage, size_t capacity);
```

Инициализирует от 2 до 4096 слотов. Структура и массив остаются по прежним адресам до закрытия сервера.

Linux64. Возвращает 1 при успехе, 0 при отказе. Регистрируйте в потоке диспетчера или до его запуска. `StyxRpc::serve`.

[Пример](../examples/disvm/reverse.c) · [Исходник](../src/program.c)
