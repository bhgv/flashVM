# styxrpc_module_slot_new_node

[English](module_slot_new_node.md) · [Заголовок](../include/styxrpc/nodes.h) · [Реализация](../src/module_slot.c)

```c
#include <styxrpc/nodes.h>
StyxRpcStatus styxrpc_module_slot_new_node(StyxRpcModuleSlot *slot,
    const char *path, unsigned mode, const StyxRpcNodeCallbacks *callbacks);
```

Подключите хостовый SDK. Передайте живой слот, путь от корня сессии с
необязательной начальной косой чертой и маску Unix от 1 до 7. Максимум
255 байт на компонент и 1024 байта на весь путь. В одном образе допускается
до 128 файлов и неявных каталогов суммарно. Служебные имена, коллизии и
неверный путь дают `STYXRPC_INVALID`. При успехе `STYXRPC_OK` публикует
весь путь и копирует callbacks. Если задан release, он освобождает context
при отзыве. После отказа context остаётся у вызывающего.

Колбэк transfer выполняется синхронно у владельца слота с
`STYXRPC_READ` или `STYXRPC_WRITE`. Заполните префикс при чтении либо
обработайте входные байты при записи и задайте `used` от нуля до `size`.
Заимствованные байты действуют во время вызова, вход записи только для чтения.
1 подтверждает префикс, 0 отклоняет операцию. При наличии права исполнения
используется селектор `STYXRPC_NODE_EXECUTE`. В буфере находятся входные
байты. Запишите ответ в его начало и задайте `used` от 0 до `size`.
[node_exec](node_exec.ru.md) вызывает один колбэк на полный запрос.
Для отзыва используйте [clear_nodes](module_slot_clear_nodes.ru.md).

Для фиксированной длины и смещений в байтах используйте
[styxrpc_module_slot_new_node_at](module_slot_new_node_at.ru.md).
