# styxrpc_module_ref_open_local

[English](module_ref_open_local.md) · [API](../include/styxrpc/module_ref.h#L54) · [Исходник](../src/module_ref_local.c#L63) · [Руководство](module_refs.ru.md)

`StyxRpc::serve`

```c
StyxRpcStatus styxrpc_module_ref_open_local(struct StyxRpcModuleSlot *slot,
    const StyxRpcModuleRef *ref, StyxRpcFile *out);
```

## Контракт

Вызывайте в потоке владельца живого слота модуля `slot`. Передайте [декодированный дескриптор](module_ref_decode.ru.md) его экспорта и обнулённый `out`. Вызов разрешает канонический путь в текущей публикации, сверяет точный Qid.path, проверяет обычное четырёхбайтовое значение и требуемые режимом дескриптора права дерева.

При успехе результат `STYXRPC_OK`, а открытый файл учитывается в слоте и записывается в `out`. Недопустимый вход, отказ или отозванный Qid дают `STYXRPC_INVALID` и сохраняют `out`. Замена модуля отзывает доступ к переменной даже при удержании файла. Используйте [чтение](module_ref_read_local.ru.md) и [запись](module_ref_write_local.ru.md) в потоке владельца. Закройте файл через `styxrpc_file_close` перед уничтожением слота. Операция не обращается к транспорту.
