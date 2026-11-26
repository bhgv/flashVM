# styxrpc_module_ref_read_local

[English](module_ref_read_local.md) · [API](../include/styxrpc/module_ref.h#L60) · [Исходник](../src/module_ref_local.c#L95) · [Руководство](module_refs.ru.md)

`StyxRpc::serve`

```c
StyxRpcStatus styxrpc_module_ref_read_local(StyxRpcFile *file, int32_t *value);
```

## Контракт

Передайте удерживаемый [локальный файл экспорта](module_ref_open_local.ru.md) и адрес записи `value` в потоке владельца слота. Вызов читает полный знаковый Int32 через колбэк переменной и возвращает `STYXRPC_OK` при успехе. Закрытый, отозванный или отказавший файл, неверная длина или NULL в качестве результата дают `STYXRPC_INVALID` и сохраняют значение вызывающего. Запрос к транспорту не выполняется.

После успешной [записи](module_ref_write_local.ru.md) другая живая ссылка читает новое значение. По завершении закройте файл через `styxrpc_file_close`.
