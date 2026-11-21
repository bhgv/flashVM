# styxunibusserverloadprofile

[English](UNIBUS-SERVER-LOAD-PROFILE.md) · [Заголовок](unibus_server.h) · [Реализация](unibus_loaded.c)

```c
DVM_Provider *styxunibusserverloadprofile(StyxUnibusServer *server);
```

Подключите `unibus_server.h` и библиотеку `styx_unibus_server`.
Передайте работающий сервер из
[styxunibusserverstartloaded](UNIBUS-SERVER-LOADED.ru.md). Вызов немедленно
возвращает заимствованный провайдер, действительный до join. Для других
режимов сервера и NULL возвращается NULL.

Выдайте в VFS владельца только запись в корневой узел провайдера. Передайте
его путь в поле `offer` конфигурации `loaded`. Владелец записывает одну
полную S-запись после связывания экспорта. Диспетчер сверяет имя и
сигнатуру и закрывает приёмник после чтения записи. Удалённый клиент
читает принятый профиль из `.interface` смонтированного каталога.
