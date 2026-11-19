# unibus-9p-bridge

Copyright (c) 2026 bhgv

[English](README.md) · [Русский](README.ru.md) · [Index](../README.ru.md)

## Что и зачем

Два клиента используют общий stdin/stdout S-записей и отдельные loopback-соединения с 9P backend. RX-dispatch не ждёт ответа backend. Worker-ответы выдаются целыми внешними записями под output-lock. Необязательное POSIX-приложение не линкует libstyx.

## Сборка и запуск

```sh
cmake -S . -B build -DUNIBUS_QUEUES=ON -DBUILD_TESTING=ON
cmake --build build -j
python3 examples/bridge/check.py build/examples/unibus-9p-bridge
ctest --test-dir build -R ^unibus-gateway$ --output-on-failure
```

## Параметры и результаты

| Argument | Values | Meaning |
| --- | --- | --- |
| argv1 | 1..65535 | Client7 loopback TCP backend port |
| argv2 | 1..65535 | Client8 loopback TCP backend port |
| stdin/stdout | binary S-record stream | Shared RX / serialized replies |

Запускайте gateway через binary pipes. check.py создаёт backend-серверы и показывает корректный обмен; не посылайте текст в stdin. EOF с частичной записью даёт код 2.

Внешние теги 7/8 назначаются схемой example.9p.v1 версии 1; внутренний тег 1 несёт raw 9P, тег 2 — ошибку backend. Frame максимум 8192 байта. У каждого клиента только один backend RPC in-flight: concurrent Tflush того же клиента не реализован этим учебным шлюзом. Разрыв клиента 7 не отменяет запросы клиента 8. Никакого автоматического replay записи. Сетевые timeout здесь 3 с на I/O.

## Исходник и справка

- [Implementation](../bridge/main.c)
- [API](../../docs/index.ru.md)
- [Protocol](../../docs/protocol.ru.md)
- [Tutorial](../../docs/tutorial.ru.md)
- [Main project](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
