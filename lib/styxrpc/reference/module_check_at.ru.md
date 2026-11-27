# styxrpc_module_check_at

[English](module_check_at.md) · [API](../include/styxrpc/module_client.h) · [styxrpc_module_check](module_check.ru.md)

`styxrpc_module_check_at(directory, imports, count, pid)`

Проверить те же импорты, число описателей и необязательный PID клиента, что у проверки по пути. Базовый неоткрытый fid выдаёт styxrpc_module_open.

Сверить сигнатуры, схемы и доступность файлов методов в удерживаемом образе.

STYXRPC_OK подтверждает все импорты. STYXRPC_INVALID означает неверный или отозванный fid, недоступный интерфейс, несовпадение контракта либо отсутствие требуемого PID. Входы заимствуются без изменений. Клиент сериализует закрытие и открытие fid. Вызов может ждать Styx.
