# styxrpc_module_ref_decode

[English](module_ref_decode.md) · [API](../include/styxrpc/module_ref.h#L43) · [Исходник](../protocol/module_ref.c#L81) · [Руководство](module_refs.ru.md)

`StyxRpc::rpc`

```c
int styxrpc_module_ref_decode(const void *bytes, size_t size, const void *scope,
    StyxRpcModuleRef *out);
```

## Контракт

Снимите тег 9 и передайте оставшиеся `size` байтов по адресу `bytes` и ожидаемую 16-байтовую область `scope` этого соединения. Вызывающий выделяет память для `out`. При успехе функция возвращает 1 и копирует полный дескриптор с завершающим NUL в пути. Чужая область, усечённое или повреждённое тело, недопустимый путь или NULL дают 0 и сохраняют `out`.

Функция не удерживает аргументы и доступна из любого потока. Перед открытием выберите [обычное подключение](module_ref_open_remote.ru.md) или [локальный слот](module_ref_open_local.ru.md) исходного владельца.
