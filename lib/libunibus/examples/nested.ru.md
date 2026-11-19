# unibus-nested

Copyright (c) 2026 bhgv

[English](nested.md) · [Русский](nested.ru.md) · [Index](README.ru.md)

## Что и зачем

Назначить схему через тег 0, затем подать вложенную запись побайтно. Внешний тег 7 и внутренний тег 1 имеют разные области видимости. Минимальный пример без allocation и физического соединения.

## Сборка и запуск

```sh
cmake -S . -B build -DUNIBUS_QUEUES=OFF -DBUILD_TESTING=ON
cmake --build build -j
./build/examples/unibus-nested
ctest --test-dir build -R ^unibus-nested$ --output-on-failure
```

## Параметры и результаты

Аргументов командной строки нет. Код 0 означает, что все проверки примера пройдены; ненулевой код указывает этап отказа в исходнике. stdout — диагностический PASS, а не произвольный двоичный transport.

Буферы 128/256 байт — учебный выбор, не предел протокола. slots содержит два назначения, реально используется одно. Проверяются tag, schema и четыре байта 21.5; MORE между фрагментами допустим. После выдачи borrowed payload нельзя перезаписывать его storage до обработки.

## Исходник и справка

- [Implementation](nested.c)
- [API](../docs/index.ru.md)
- [Protocol](../docs/protocol.ru.md)
- [Tutorial](../docs/tutorial.ru.md)
- [Main project](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
