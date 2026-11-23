# styxintarrayread

[English](intarrayread.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/indexed_variable.h>
int styxintarrayread(StyxIntArray *array, uint64_t generation, uint64_t offset, void *bytes, size_t *size);
```

Читает знаковый Int32 в действующем поколении селектора. Передайте ёмкость через size; при успехе там число байтов. EOF со смещения четыре.

`StyxRpc::rpc`.

[Пример](../examples/disvm/reverse.c) · [Исходник](../protocol/indexed_variable.c)
