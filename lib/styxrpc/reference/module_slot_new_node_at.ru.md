# styxrpc_module_slot_new_node_at

[English](module_slot_new_node_at.md) · [Заголовок](../include/styxrpc/nodes.h) · [Реализация](../src/module_slot.c)

```c
#include <styxrpc/nodes.h>
StyxRpcStatus styxrpc_module_slot_new_node_at(StyxRpcModuleSlot *slot,
    const char *path, unsigned mode, uint64_t length,
    const StyxRpcPositionedNodeCallbacks *callbacks);
```

Слинкуйте hosted SDK. Публикуйте в живой `slot` из его
потока-владельца. Правила `path`, прав Unix `mode`, результата и
времени жизни совпадают с [styxrpc_module_slot_new_node](module_slot_new_node.ru.md).
`callbacks` содержит функцию обмена, необязательную функцию
освобождения и её контекст. SDK копирует таблицу и при успешной
публикации принимает контекст во владение, если указана функция
освобождения. `length` — число байтов от нуля до
`STYXRPC_NODE_OFFSET_MAX` (2⁵³−1), видимое через stat. Создания
хранилища соответствующего размера не происходит.

Функция обмена получает `context`, вид операции, абсолютное
смещение `offset` в байтах, заимствованный буфер и число байтов
`size`. Укажите `*used` от нуля до `size` и верните единицу для
подтверждения префикса. Чтение за EOF возвращает ноль без вызова.
Пересечение EOF при чтении уменьшает предоставленный буфер.
Фрагмент записи за EOF отклоняется до вызова. Данные записи доступны
для чтения. Пустой файловый запрос подтверждает ноль байтов без
колбэка. Полный запрос исполнения не зависит от длины файла и
получает смещение ноль.

Например, `styxrpc_module_slot_new_node_at(slot, "app/page", 4, 4096, &ops)`
публикует файл на 4096 байтов для чтения через функцию из `ops`.
При чтении с байта 4095 передаётся буфер не больше одного байта.
Для отзыва вызовите [clear_nodes](module_slot_clear_nodes.ru.md)
в потоке-владельце слота.
