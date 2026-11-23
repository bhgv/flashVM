# styxrpc_dir_entry

[English](dir_entry.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/vfs.h>
StyxRpcStatus styxrpc_dir_entry(const StyxRpcVfs *ops, void *context, uint64_t directory, size_t index, StyxRpcEntry *entry);
```

Вызывает перечисление из ops с заимствованным context, ID родителя directory и индексом потомка index от нуля. При OK заполняет entry. Имя живёт до следующего перечисления. MORE означает конец, INVALID — неверное имя или отсутствие колбэка.

C99. Вызывайте из диспетчера, сериализующего операции одного fid. `StyxRpc::rpc`.

[Пример](../examples/device.c) · [Исходник](../src/vfs.c)
