# styxrpc_argument_scope_activate

[English](argument_scope_activate.md) · [Объявление](../include/styxrpc/channel_arguments.h#L47) · [Реализация](../src/channel_arguments.c) · [Сценарий](channel_argument_scopes.ru.md)

Подключите `StyxRpc::launch`.

```c
#include <styxrpc/channel_arguments.h>
int styxrpc_argument_scope_activate(StyxRpcArgumentScope *scope);
```

Используйте с собственным прямым транспортом, который уже сверил точное предложение и готов к записи. `scope` — область подготовки. При успехе функция возвращает 1 и публикует весь набор ключей. Неверное состояние или метаданные очереди дают 0. Локальные операции с памятью очереди возобновляйте после [close](argument_scope_close.ru.md). Стандартный [invoke](argument_scope_invoke.ru.md) активирует её сам.

[Проверка C](../tests/channel_arguments.c)
