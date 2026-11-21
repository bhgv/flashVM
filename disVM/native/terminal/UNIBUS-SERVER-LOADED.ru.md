# styxunibusserverstartloaded

[English](UNIBUS-SERVER-LOADED.md) · [Заголовок](unibus_server.h) · [Реализация](unibus_loaded.c)

```c
StyxUnibusServer *styxunibusserverstartloaded(
    RemotePhy *channel, const char *module, const char *name);
```

Подключите `unibus_server.h` и библиотеку `styx_unibus_server`.
Передайте свой соединённый канал Styx и два заимствованных компонента
пути. `module` задаёт каталог внутри `mounted`; `name` задаёт функцию
и занимает до 48 байтов UTF-8. Имена должны быть доступны до возврата.
Конструктор принимает канал во владение и возвращает сервер либо NULL
при неверных параметрах или ошибке запуска.

Конструктор резервирует один экспорт. Встраивающая сторона выдаёт VM
владельца [служебный приёмник профиля](UNIBUS-SERVER-LOAD-PROFILE.ru.md),
чтение запросов и запись ответов из `styxunibusservermailbox`. Запустите
[общий сервис](../../runtime/SCALAR-OWNER-SERVICE.ru.md#профиль-из-исходного-load)
в пространстве владельца с путями этих трёх файлов. См.
[пример запуска](tests/scalar/pair.c).

Открытие функции или её `.interface` ждёт связывания до трёх секунд.
После истечения срока можно открыть файл снова. Отказ или неверная
запись закрывает доступ до конца сессии. Принятое предложение задаёт
диспетчеризацию до завершения сервера. После закрытия канала освободите
сервер через `styxunibusserverjoin`.

Явную публикацию и каталоги нескольких экспортов описывает
[styxunibusserverstartmounted](SCALAR-FUNCTION-MOUNTED.ru.md).

Для нескольких экспортов используйте
[styxunibusserverstartloadedset](UNIBUS-SERVER-LOADED-SET.ru.md).

См. [styxunibusserverstartscopedset](UNIBUS-SERVER-SCOPED-SET.ru.md) для вызовов по схеме с контекстом клиента.
