# styxrpc_module_channels_revoke

[English](module_channels_revoke.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/module_channels.h>
int styxrpc_module_channels_revoke(StyxRpcModuleChannels *channels,
    const unsigned char token[STYXRPC_CHANNEL_TOKEN_SIZE]);
```

Отзовите точный 16-байтовый `token` в реестре `channels`. Действующий
токен даёт 1. Отсутствующий токен или неверные аргументы дают 0. Новый
вход по токену сразу запрещается. Открытые по нему дескрипторы закрываются
по одному разу. Старые fid отклоняют ввод-вывод, а последующий clunk
освобождает их обёртку. Слот доступен для новой выдачи. Другие токены
продолжают обслуживать свои дескрипторы.

Вызывайте при закрытии fid выдавшей функции и при ошибках после публикации.
Диспетчер выполняет колбэки закрытия синхронно. Если ресурс использует
`styxrpc_channel_fid_close`, ожидающие операции планировщика отменяются.
Уже выполненные операции сохраняют эффект. Контекст ресурса можно
освободить после возврата. Вызывайте функцию вне колбэков целевого ресурса.

Библиотека `StyxRpc::serve`.

[Учебный пример](../examples/module_channels.ru.md) · [styxrpc_module_channels_publish](module_channels_publish.ru.md) · [Исходник](../src/module_channels.c)
