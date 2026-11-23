# styxrpc_channel_element_remap

[English](channel_element_remap.md) · [Объявление](../include/styxrpc/channel_route.h) · [Реализация](../src/channel_remap.c) · [Сценарий](channel_argument_scopes.ru.md)

Подключите `<styxrpc/channel_route.h>` и линкуйте `StyxRpc::rpc`.

```c
StyxRpcStatus styxrpc_channel_element_remap(const char *shape, const void *bytes, size_t size,
    const StyxRpcChannelKeyMap *keys, size_t count, void *output, size_t capacity, size_t *used);
```

| Параметр | Контракт |
| --- | --- |
| `shape` | Точный список типов в скобках с нулевым завершением для полного элемента канала. |
| `bytes`, `size` | Один типизированный элемент для копирования, до `STYXRPC_MANAGED_LIMIT` байт. |
| `keys`, `count` | От нуля до восьми пар положительных 31-битных ключей. Оба столбца уникальны. Для каждого исходного ключа текущей шины нужна пара. Для пустой карты допустим NULL. |
| `output`, `capacity`, `used` | Отдельный выходной буфер, его ёмкость до `STYXRPC_MANAGED_LIMIT` байт и точный размер результата при успехе. |

Заменяет ключи листьев на обычные контекстные ключи и пересчитывает длины и контрольные суммы окружающих записей. Nil и поля со значениями сохраняются. `STYXRPC_OK` означает полный кадр. Неверные кадры или карты, отдельно адресованные маршруты, нехватка памяти и недостаточная ёмкость дают `STYXRPC_INVALID`. Функция работает без ввода-вывода. [Подготовьте новые ключи](argument_scope_nested.ru.md) на нужной шине вызывающего до активации. [Пример](../tests/channel_arguments.c)
