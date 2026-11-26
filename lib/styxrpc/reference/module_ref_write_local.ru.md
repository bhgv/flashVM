# styxrpc_module_ref_write_local

[English](module_ref_write_local.md) · [API](../include/styxrpc/module_ref.h#L61) · [Исходник](../src/module_ref_local.c#L109) · [Руководство](module_refs.ru.md)

`StyxRpc::serve`

```c
StyxRpcStatus styxrpc_module_ref_write_local(StyxRpcFile *file, int32_t value);
```

## Контракт

Передайте доступный для записи [локальный файл экспорта](module_ref_open_local.ru.md) и знаковый Int32 `value` в потоке владельца. Вызов передаёт колбэку переменной полное значение со смещения ноль. Успех даёт `STYXRPC_OK`. Закрытый, отозванный, доступный только для чтения или отказавший файл даёт `STYXRPC_INVALID`. Вызов не обращается к транспорту. Файл остаётся у вызывающего.

Запишите, например, `-73`. Следующее [чтение](module_ref_read_local.ru.md) через любую живую ссылку на экспорт увидит это значение.
