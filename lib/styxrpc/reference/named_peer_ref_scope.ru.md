# styxrpc_named_peer_ref_scope

[English](named_peer_ref_scope.md) · [API](../include/styxrpc/named_peer.h#L43) · [Исходник](../src/named_peer.c#L291) · [Руководство](module_refs.ru.md)

`StyxRpc::serve`

```c
int styxrpc_named_peer_ref_scope(StyxRpcNamedPeer *peer, void *scope);
```

## Контракт

Вызовите на стороне forward с живым [хостом с областью](named_peer_host_scoped.ru.md) и 16-байтовым выходным буфером `scope`. После объявления поддержки во встречном подключении службы имён через `STYXRPC_MODULE_REF_ATTACH` вызов возвращает 1 и копирует случайные байты. При отсутствии поддержки или NULL в аргументах возвращает 0 и сохраняет буфер.

Проверка объявленной встречной возможности не блокируется. Сохраняйте `peer` до завершения вызова. Полученную область используйте для [кодирования](module_ref_encode.ru.md) и [декодирования](module_ref_decode.ru.md) ссылок.
