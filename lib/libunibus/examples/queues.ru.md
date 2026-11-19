# unibus-queues

Copyright (c) 2026 bhgv

[English](queues.md) · [Русский](queues.ru.md) · [Index](README.ru.md)

## Что и зачем

Pthread-потребитель ждёт только клиента 7, пока диспетчер выдаёт клиенту 8 готовую запись. Barrier задаёт совместное время жизни без sleep. Затем main выдаёт клиенту 7, делает join и закрывает/освобождает оба engine.

## Сборка и запуск

```sh
cmake -S . -B build -DUNIBUS_QUEUES=ON -DBUILD_TESTING=ON
cmake --build build -j
./build/examples/unibus-queues
ctest --test-dir build -R ^unibus-queues$ --output-on-failure
```

## Параметры и результаты

Аргументов командной строки нет. Код 0 означает, что все проверки примера пройдены; код 1 означает отказ; stderr содержит название этапа. stdout — диагностический PASS, а не произвольный двоичный transport.

Параметры [styxordercreate](../docs/reference/styxordercreate.ru.md): максимум сообщения 256, окно 16; wait клиента 7 ограничен 5000 мс. Barrier подтверждает запуск worker, но не точный момент входа в wait. При любой ошибке до создания worker освобождаются уже созданные engines и barrier. После создания receiver закрывается перед join; готовые записи доступны для дочитывания. Проверка unibus-example-failures внедряет четыре отказа частичной инициализации. Неожиданная ошибка join завершает учебный процесс через _Exit: без подтверждённого завершения worker нельзя освобождать заимствованное им состояние. Встроенное приложение должно выбрать свою политику такого фатального отказа. Это параметры примера, не политика всех клиентов. Старое C-имя styxorder сохраняется для совместимости исходников, зависимости от libstyx нет.

## Исходник и справка

- [Implementation](queues.c)
- [API](../docs/index.ru.md)
- [Protocol](../docs/protocol.ru.md)
- [Tutorial](../docs/tutorial.ru.md)
- [Main project](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
