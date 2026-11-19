# Живой реестр окон/контролов

Copyright (c) 2026 bhgv

[English](README.md) · [Index](../README.ru.md)

## Назначение и выбор модели

Backend содержит небольшой нативный реестр без GUI toolkit. Запись add/remove в ctl сразу публикует/скрывает записи. Стабильные ID не переиспользуются. Скрытие имени не удаляет открытый объект: catalog_pin/catalog_unpin удерживают его через open/close исходного сервера. Запросы заменяются обращениями к реальному реестру окон/контролов приложения.

## Сборка и запуск

```sh
cmake -S . -B build -DSTYX_HOSTED=ON -DBUILD_TESTING=ON  # Hosted example targets / Нативные примеры
cmake --build build -j  # Build the library and examples / Собрать библиотеку и примеры
./build/examples/styx-live 5640  # Keep running in terminal1 / Оставить в терминале1
```

```sh
./build/examples/styx-cat 5640 window-100/title  # Read through original native 9P client / Читать нативным 9P-клиентом
ctest --test-dir build -R '^styx-live$' --output-on-failure  # Independent wire acceptance / Независимая wire-проверка
```

## Параметры

| Параметр | Варианты | Смысл |
| --- | --- | --- |
| argv1 | `0..65535`, default `0` | TCP-порт;0 выбирает свободный. Первая stdout-строка — реальный порт. |
| argv2 | unused | Источник явно задаёт оператор; клиент не передаёт host-пути. |
| names | `ctl; window-ID/title; window-ID/button` | Пространство имён этого примера. |
| file sample | `window-100/title` | Ожидаемый текст: `Live window` |

## Как проходят запросы

Twalk разрешает один или несколько компонентов в fid. Topen проверяет режим и удерживает объект. Tread файла использует удерживаемый ID и offset. Tread каталога кодирует только целые Dir непосредственных детей; offset измеряется байтами. Tclunk/разрыв вызывает cleanup. Между запросами дерево может меняться; повторный обход перед каждым read/write не добавляется.

## Границы и изменение backend

Общий CatalogRow содержит имя до127 UTF-8 байт и demonstration text до2047 байт; страница содержит максимум16 детей. Для фотографий/больших данных заменить текстовый read на byte-stream/read по offset. Это не требование 9P хранить файл целиком. Списки, изменившиеся между страницами, могут требовать rewind offset0. Backend-вызовы примера синхронны; production asynchronous I/O использует deferred-read API.

TSV-пример ограничен128 ключами; он перечитывает один плоский набор, не рекурсивное дерево. Live-пример имеет16 мест для видимых/удерживаемых окон. HTTP-helper ограничивает ответ128KiB, socket timeout3с и process alarm5с; IDs должны точно сохранять uint64, без округления через JavaScript double.

## Исходники и справка

- [Server callbacks](../server.c), [catalog_query / pin / unpin contract](../catalog.h), [backend implementation](../live.c).
- [styxinit](../../docs/reference/styxinit.ru.md), [convD2M](../../docs/reference/convD2M.ru.md), [convM2S](../../docs/reference/convM2S.ru.md).
- [Original client](../client.c), [independent wire client](../wire_client.py), [acceptance](../check.py).
- [Main-project native Styx help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md).

## Изменение реестра

```python
from wire_client import Client  # PYTHONPATH=examples; standard 9P client / Стандартный клиент
c = Client(5640)  # Own one connection / Владеть соединением
c.write('ctl', 'add')  # Publish a new window / Опубликовать новое окно
print(c.directory('/'))  # Observe current names / Увидеть текущие имена
c.write('ctl', 'remove 100')  # Hide name, retain open objects / Скрыть имя, удержать open-объекты
c.close()  # Retire fids and connection / Освободить fid и соединение
```
