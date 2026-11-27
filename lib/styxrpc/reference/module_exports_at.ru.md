# styxrpc_module_exports_at

[English](module_exports_at.md) · [API](../include/styxrpc/module_client.h) · [styxrpc_module_exports](module_exports.ru.md)

`styxrpc_module_exports_at(directory, pid, out)`

Прочитать компилированные контракты и найти вызываемые методы относительно заимствованного неоткрытого каталога. PID имеет тот же смысл, что при поиске по пути.

Найти экспорты образа, уже выбранного при открытии.

STYXRPC_OK выдаёт проверенные экспорты в out. Не перемещайте out после успеха, поскольку описатели ссылаются на свои имена внутри него. STYXRPC_INVALID сохраняет out. Клиент сериализует открытие и закрытие fid. Возможны ожидания Styx.
