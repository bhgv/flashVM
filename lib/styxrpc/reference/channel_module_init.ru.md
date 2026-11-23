# styxrpc_channel_module_init

[English](channel_module_init.md) · [Объявление](../include/styxrpc/channel_module.h#L46) · [Реализация](../src/channel_module.c) · [Описание аренды](channel_leases.ru.md)

Подключите `StyxRpc::serve`.

```c
#include <styxrpc/channel_module.h>
StyxRpcStatus styxrpc_channel_module_init(StyxRpcChannelModule *module, const char *name,
    const StyxRpcChannelExport *exports, size_t count, StyxRpcModuleChannels *registry);
```

Подготовьте память `module` с базовым именем `name`, `count` различными компилированными описателями в `exports` и инициализированным `registry` для данного модуля и сессии. Каждый описатель принимает вход по значению и один возвращаемый канал с элементами по значению. Передайте `module` как контекст `styxrpc_channel_module_tree`. Модуль публикует компилированное предложение, штатные маркеры и `.unibus` с проверкой полномочий.

`STYXRPC_OK` означает готовность модуля. Неверная настройка даёт `STYXRPC_INVALID`. Модуль, имена, описатели, реестр и его источник случайных байтов живут до уничтожения сервера. Диспетчер вызывает колбэки последовательно. Успешный колбэк возвращает положительный ключ и ресурс новой аренды либо ноль для nil. Если передан `release`, очистка вызывает его один раз при отказе публикации или отзыве. Отзыв закрывает ожидающие операции до очистки.

[Исполняемая проверка](../tests/module_channels.c)
