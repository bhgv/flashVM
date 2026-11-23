# styxintarrayclose

[English](intarrayclose.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/indexed_variable.h>
void styxintarrayclose(StyxIntArray *array, uint64_t generation);
```

Освобождает точное поколение селектора. Соответствующие fid значений теряют доступ. Передавайте только поколение, которым владеет селектор.

`StyxRpc::rpc`.

[Пример](../examples/disvm/reverse.c) · [Исходник](../protocol/indexed_variable.c)
