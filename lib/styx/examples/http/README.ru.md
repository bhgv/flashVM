# Ленивая постраничная HTTP-иерархия

Copyright (c) 2026 bhgv

[English](README.md) · [Index](../README.ru.md)

## Назначение и выбор модели

Каждый stat/list вызывает HTTP-adapter с выбранным стабильным ID и одной страницей непосредственных детей. На старте дерево не загружается. Настоящий HTTP-fixture вычисляет условную иерархию с миллиардами узлов без её хранения. Это воспроизводимый тест, не реальные географические сведения. JSON-сервис заменяется adapter настоящего интернет/БД источника.

## Сборка и запуск

```sh
cmake -S . -B build -DSTYX_HOSTED=ON -DBUILD_TESTING=ON  # Hosted example targets / Нативные примеры
cmake --build build -j  # Build the library and examples / Собрать библиотеку и примеры
python3 examples/http_source.py 8080  # Run in another terminal / Запустить в другом терминале
./build/examples/styx-http 5640 http://127.0.0.1:8080/node  # Keep running in terminal1 / Оставить в терминале1
```

```sh
./build/examples/styx-cat 5640 Exampleland/city-2/district-3/street-4/house-5/room-6/info  # Read through original native 9P client / Читать нативным 9P-клиентом
ctest --test-dir build -R '^styx-http$' --output-on-failure  # Independent wire acceptance / Независимая wire-проверка
```

## Параметры

| Параметр | Варианты | Смысл |
| --- | --- | --- |
| argv1 | `0..65535`, default `0` | TCP-порт;0 выбирает свободный. Первая stdout-строка — реальный порт. |
| argv2 | HTTP(S) endpoint URL | Источник явно задаёт оператор; клиент не передаёт host-пути. |
| names | `country/city/district/street/house/room/info` | Пространство имён этого примера. |
| file sample | `Exampleland/city-2/district-3/street-4/house-5/room-6/info` | Ожидаемый текст: `Synthetic information for room-6` |

## Как проходят запросы

Twalk разрешает один или несколько компонентов в fid. Topen проверяет режим и удерживает объект. Tread файла использует удерживаемый ID и offset. Tread каталога кодирует только целые Dir непосредственных детей; offset измеряется байтами. Tclunk/разрыв вызывает cleanup. Между запросами дерево может меняться; повторный обход перед каждым read/write не добавляется.

## Границы и изменение backend

Общий CatalogRow содержит имя до127 UTF-8 байт и demonstration text до2047 байт; страница содержит максимум16 детей. Для фотографий/больших данных заменить текстовый read на byte-stream/read по offset. Это не требование 9P хранить файл целиком. Списки, изменившиеся между страницами, могут требовать rewind offset0. Backend-вызовы примера синхронны; production asynchronous I/O использует deferred-read API.

TSV-пример ограничен128 ключами; он перечитывает один плоский набор, не рекурсивное дерево. Live-пример имеет16 мест для видимых/удерживаемых окон. HTTP-helper ограничивает ответ128KiB, socket timeout3с и process alarm5с; IDs должны точно сохранять uint64, без округления через JavaScript double.

## Исходники и справка

- [Server callbacks](../server.c), [catalog_query / pin / unpin contract](../catalog.h), [backend implementation](../http_catalog.c).
- [styxinit](../../docs/reference/styxinit.ru.md), [convD2M](../../docs/reference/convD2M.ru.md), [convM2S](../../docs/reference/convM2S.ru.md).
- [Original client](../client.c), [independent wire client](../wire_client.py), [acceptance](../check.py).
- [Main-project native Styx help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md).

## HTTP backend schema

`GET endpoint?op=list&id=0&start=0&limit=16` returns a JSON array. `op=stat` returns zero or one row. Each row has exact unsigned64 `id`/`parent`, numeric standard `mode`, UTF-8 `name`, optional text `value`. The source owns stable identities and access policy; changing a filename must not alias an unrelated held ID.

[HTTP adapter](../http_query.py) · [HTTP fixture](../http_source.py).
