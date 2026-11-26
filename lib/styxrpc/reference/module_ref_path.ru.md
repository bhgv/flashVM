# styxrpc_module_ref_path

[English](module_ref_path.md) · [API](../include/styxrpc/module_ref.h#L33) · [Исходник](../protocol/module_ref.c#L28) · [Руководство](module_refs.ru.md)

`StyxRpc::rpc`

```c
int styxrpc_module_ref_path(const char *path);
```

## Контракт

Передайте путь с завершающим NUL. Вызов возвращает 1 для формы `mounted/<образ>/.var/<переменная>`. Каждое имя занимает от 1 до 255 байтов, не содержит косых черт и отличается от `.` и `..`. Начальная косая черта, пустое имя или NULL дают 0. Строка заимствуется на время вызова. Функция не делает запросов VFS и доступна из любого потока.

Путь `mounted/counters.dis/.var/count` проходит проверку. Для проверки самого узла выполните [открытие с Qid](vfs_int_open_identified.ru.md).
