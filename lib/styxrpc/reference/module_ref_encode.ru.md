# styxrpc_module_ref_encode

[English](module_ref_encode.md) · [API](../include/styxrpc/module_ref.h#L38) · [Исходник](../protocol/module_ref.c#L49) · [Руководство](module_refs.ru.md)

`StyxRpc::rpc`

```c
size_t styxrpc_module_ref_encode(const StyxRpcModuleRef *ref, const void *scope,
    void *bytes, size_t capacity);
```

## Контракт

Укажите в `ref->node` ненулевой Qid.path открытого экспорта, в `owner` роль `STYXRPC_MODULE_REF_FORWARD` или `STYXRPC_MODULE_REF_REVERSE`, а в `writable` число 0 или 1. Дескриптор содержит [канонический путь](module_ref_path.ru.md) с завершающим NUL. Передайте согласованную 16-байтовую область `scope` и выходной буфер `bytes` ёмкостью `capacity` байтов. Входные данные и буфер должны занимать разные участки памяти.

Результат равен длине тела в байтах. Ноль означает недопустимый вход или недостаточную ёмкость. При этом буфер сохраняется. Буфера `STYXRPC_MODULE_REF_WIRE_MAX` достаточно для любого допустимого тела. Для передачи добавьте перед телом тег канала 9. Функция заимствует входные данные на время вызова, не делает I/O и доступна из любого потока.
