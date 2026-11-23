# Связь FlashVM и DisVM через libstyxrpc

[English](STYX_RPC.md) · [Импорт функции int](STYX_RPC_IMPORT_INT.ru.md) · [Экспорт функции int](STYX_RPC_EXPORT_INT.ru.md)

FlashVM и DisVM обмениваются вызовами через libstyxrpc поверх Styx. Хост
передаёт SDK Styx уже разрешённое подключение. Модуль публикует компилированный
интерфейс `.interface` в смонтированной VFS. Каждый вызов использует новый fid
функции и передаёт S-записи UniBus. По соединению передаются значения. Объекты,
сборщик мусора и потоки исполнения принадлежат каждой VM отдельно.

Текущий адаптер FlashVM связывает числа ActionScript с импортами и экспортами
точной знаковой сигнатуры `fn(int): int`. Импорт становится обычной вызываемой
глобальной функцией AS. В DisVM обычный типизированный Limbo `load` загружает
экспорты на FlashVM. Адаптер находится в цели `flashvm_styxrpc`, а независимый
SDK libstyxrpc доступен любой программе на C. Внешний клиент и загрузчик DisVM
используют общую проверку скалярного интерфейса, сигнатур и формат записей.
Будущие расширения внутри DisVM должны сохранять этот общий контракт.

## Сборка

Установите хостовый SDK libstyxrpc и его зависимости. Если SDK установлен
в `/path/to/prefix`, настройте FlashVM из корня рабочего пространства:

```sh
cmake -S flashVM -B build/flash-styx \
    -DFVM_ENABLE_STYXRPC=ON -DCMAKE_PREFIX_PATH=/path/to/prefix
cmake --build build/flash-styx --target flashvm_styxrpc
```

Подключите в приложении цель `flashvm_styxrpc` и заголовок
[flashvm/styxrpc.h](../include/flashvm/styxrpc.h). Для автоматических проверок
оставьте `FVM_BUILD_TESTS=ON`, соберите `flashvm_tests_styxrpc_host` и
передайте его путь в `STYXRPC_FLASHVM_TEST_HOST` при настройке
[приёмочной сборки libstyxrpc](https://github.com/bhgv/flashVM/blob/master/lib/styxrpc/examples/README.ru.md#сквозная-приёмка-disvm).
Тест запускает обе VM, проверяет вызовы в обе стороны и восстановление после
ошибок в интерпретаторе и JIT DisVM.

Для синхронных RPC используйте выделенный контекст-владелец FlashVM.
Текущий адаптер охватывает скалярный `int`. Управляемые аргументы и результаты,
ресурсы VFS, каналы и запуск входят в более широкий интерфейс libstyxrpc.
Их привязки к ActionScript и перенос существующих внутрипроцессных связей
редактора остаются отдельной работой.
