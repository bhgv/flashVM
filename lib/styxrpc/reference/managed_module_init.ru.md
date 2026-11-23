# styxrpc_managed_module_init

[English](managed_module_init.md) · [Заголовок](../include/styxrpc/managed_module.h) · [Реализация](../src/managed_module.c)

Подключите `styxrpc/managed_module.h` и цель `StyxRpc::serve`.

```c
StyxRpcStatus styxrpc_managed_module_init(StyxRpcManagedModule *module,
    const char *name, const StyxRpcManagedExport *exports, size_t count);
```

Инициализируйте выделенный приложением `module` с 1–8 разными описателями.
Для каждого укажите точное имя функции, компилированную сигнатуру, схему и
синхронный колбэк. `name` — один компонент пути длиной до 255 байтов.
При успехе передайте `&styxrpc_managed_module_tree` в
[styxrpc_server_create](server_create.ru.md) с модулем как контекстом.
Получится путь `mounted/<name>`. Описатели, имена и контекст должны жить
без изменений до уничтожения сервера. Неверное имя, дубликат, сигнатура
или схема приводят к `STYXRPC_INVALID`.

При открытии файла функции выделяются отдельные буферы запроса, ответа
и рабочей области до 64 КиБ каждый. Записи собирают проверенный запрос
и один раз вызывают колбэк в потоке диспетчера. Колбэк возвращает результат
или удалённую ошибку. Если он откладывает выполнение, диспетчер отменяет
ожидание и отклоняет запись. Для асинхронного завершения используйте
[низкоуровневые managed open и finish](managed_open.ru.md).
Clunk и обрыв освобождают буферы fid. Колбэки исполняются в потоке владельца
приложения.
