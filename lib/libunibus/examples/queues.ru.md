# unibus-queues

Copyright (c) 2026 bhgv

[English](queues.md) · [Русский](queues.ru.md) · [Index](README.ru.md)

## Что и зачем

Pthread-потребитель ждёт только клиента 7, пока диспетчер выдаёт клиенту 8 запись. Barrier задаёт совместное время жизни без sleep, но не точный момент wait admission. Затем main выдаёт клиенту 7 и освобождает worker/ресурсы.

## Сборка и запуск

```sh
cmake -S . -B build -DUNIBUS_QUEUES=ON -DBUILD_TESTING=ON
cmake --build build -j
./build/examples/unibus-queues
ctest --test-dir build -R '^unibus-queues$' --output-on-failure
```

## Параметры и результаты

Аргументов командной строки нет. Код 0 означает точный ожидаемый результат; код 1 — отказ с названием этапа в stderr. stdout содержит диагностический PASS, не двоичные байты носителя.

[styxordercreate](../docs/reference/styxordercreate.ru.md) задаёт максимум сообщения 256 и окно 16. Wait клиента 7 ограничен 5000 мс; это параметры примера, не политика всех клиентов. Старое C-имя styxorder сохраняется для совместимости исходников и не создаёт зависимости от libstyx.

При setup failure уже созданные engines/barrier освобождаются. После создания worker receiver закрывается до join; готовые записи остаются доступны для drain. unibus-example-failures внедряет четыре ошибки частичной инициализации. Неожиданная ошибка join завершает учебный процесс через _Exit: без подтверждённого завершения worker нельзя освобождать его заимствованное состояние. Встроенное приложение выбирает собственную политику такого фатального отказа.

## Исходник и справка

- [Implementation](queues.c)
- [API](../docs/index.ru.md)
- [Protocol](../docs/protocol.ru.md)
- [Tutorial](../docs/tutorial.ru.md)
- [Main project](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
