# Пример отключённой сессии отладчика

[English](README.md) · [Графические панели](../../../DEBUGGER.ru.md)

[Прокомментированный acceptance](../../tests/idedebug_test.b) создаёт настоящую
Limbo-сессию, запрашивает её нулевые capabilities и проверяет честный отказ
Attach/Pause/Continue/Step/Next. Старый Shutdown не должен завершить сессию;
текущий получает терминальное подтверждение. Никаких вымышленных кадров цели.

В установленном пакете запускайте `examples/debugger-scaffold/acceptance.b`
поставляемым NC64-компилятором по командам [English README](README.md).
Ожидается `PASS disconnected debugger`. Настоящие MovieClip-панели открываются
через Debug → Open panels на общем столе. Исполнение/отладка цели пока не подключены.
