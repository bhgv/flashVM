# styxunibusserverloadprofileat

[English](UNIBUS-SERVER-LOAD-PROFILE-AT.md) · [Заголовок](unibus_server.h) · [Реализация](unibus_loaded.c)

```c
DVM_Provider *styxunibusserverloadprofileat(StyxUnibusServer *server, size_t index);
```

Подключите `unibus_server.h` и библиотеку `styx_unibus_server`. Передайте
[сервер связанных экспортов](UNIBUS-SERVER-LOADED-SET.ru.md) и индекс
экспорта с нуля в исходном массиве имён. Вызов сразу возвращает
заимствованный провайдер до join, либо NULL для неверного индекса,
отсутствующего сервера или сервера с явно заданными профилями.

Смонтируйте его корень на запись в пространстве владельца и передайте
этот путь в соответствующее поле `offer`. Каждый приёмник получает одну
полную запись от общего сервиса. Индекс ноль доступен также через
[styxunibusserverloadprofile](UNIBUS-SERVER-LOAD-PROFILE.ru.md).
См. [настройку разрешений в примере](tests/scalar/pair.c).
