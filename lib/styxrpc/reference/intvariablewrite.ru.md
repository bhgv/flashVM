# styxintvariablewrite

[English](intvariablewrite.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/variable_file.h>
int styxintvariablewrite(const StyxIntVariable *variable, uint64_t offset, const void *bytes, size_t size);
```

Принимает ровно четыре байта в сетевом порядке со смещения ноль и один раз вызывает store со знаковым значением. Возвращает 1 при успехе или 0 при неверном смещении, размере или ошибке колбэка.

C99. `StyxRpc::rpc`.

[Пример](../examples/disvm/reverse.c) · [Исходник](../protocol/variable_file.c)
