# Кадры терминала UniBus

[English](unibus_terminal.md) · [unibus_terminal.h](../include/styxrpc/unibus_terminal.h)

Адаптер `StyxRpcTerminal` использует один открытый порт с двумя схемами.
Запись `_inferno.credit` версии 1 имеет пустое тело и разрешает приём
одного кадра `_inferno.fvd1` версии 1. Кадр с заголовком fvd1 занимает
53–1077 байт. Разбор содержимого выполняет приложение. Поток владельца
извлекает байты из `terminal.bytes`, затем обнуляет `terminal.size`
и снова выдаёт разрешение. Повтор разрешения до отправки или кадр без
выданного разрешения завершает работу порта.

Обнулите структуру, заполните [types](terminal_types.ru.md) и закройте
старый порт перед повторным использованием структуры. Закрепите точный
открытый порт в `terminal.port`, [выдайте кредит](terminal_allow.ru.md)
и пользуйтесь [отправкой](terminal_send.ru.md).

[Native pair test](../tests/flashvm/native_terminal.b)
