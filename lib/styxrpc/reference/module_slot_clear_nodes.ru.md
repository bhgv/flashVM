# styxrpc_module_slot_clear_nodes

[English](module_slot_clear_nodes.md) · [Заголовок](../include/styxrpc/nodes.h) · [Реализация](../src/module_slot.c)

```c
#include <styxrpc/nodes.h>
void styxrpc_module_slot_clear_nodes(StyxRpcModuleSlot *slot);
```

Отозвать все файлы с колбэками и их неявные каталоги. Контекст каждого
колбэка освобождается через необязательный хук release. Старые открытые fid
отклоняют последующие операции и доступны для clunk. Управление плеером
и образы модулей имеют свой жизненный цикл.

Вызов выполняется у владельца слота вне колбэков узлов. Допустимы NULL и
повторные вызовы. I/O транспорта не выполняется.
