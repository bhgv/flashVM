# styxrpc_channel_handlers_create

[English](channel_handlers_create.md) · [Объявление](../include/styxrpc/channel_handlers.h) · [Реализация](../src/channel_handlers.c) · [Сценарий](channel_handler_modules.ru.md) · [Проверка C](../tests/channel_handlers.c)

Подключите `<styxrpc/channel_handlers.h>` и линкуйте хостовую цель C99 `StyxRpc::serve` с поддержкой потоков POSIX.

```c
StyxRpcChannelHandlers *styxrpc_channel_handlers_create(const char *name,
    const StyxRpcChannelFunction *functions, size_t count);
```

Создаёт дерево функций и очередь последовательных колбэков.

| Параметр | Контракт |
| --- | --- |
| `name` | Обязательное имя модуля внутри `mounted`. Непустой компонент длиной до 255 байт без косых черт. |
| `functions` | Обязательный неизменяемый массив. Каждый элемент хранит уникальное компилированное имя, 32-битную сигнатуру Limbo, схему, колбэк и необязательный контекст колбэка. Схема требует хотя бы один аргумент-канал. Элементы каналов и результат используют типы по значению. Допустим void. |
| `count` | Число описателей от 1 до `STYXRPC_MAX_EXPORTS`. |

Возвращает собственный модуль или NULL при неверных описателях, превышении размера интерфейса, нехватке памяти либо ресурсов потоков. Модуль заимствует имена, описатели и контексты до уничтожения. Колбэк записывает типизированные дочерние записи в переданный буфер и устанавливает число байт при `STYXRPC_OK`. Ошибка становится удалённой ошибкой вызова. Очередь вмещает 32 полных ожидающих вызова и один активный. FID функции принимает один полный запрос до `STYXRPC_MANAGED_LIMIT` байт. Переполнение приводит к отказу финальной записи.

[`step`](channel_handlers_step.ru.md) · [`stop`](channel_handlers_stop.ru.md) · [`destroy`](channel_handlers_destroy.ru.md)
