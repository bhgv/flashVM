# Динамическое пространство имён из HTTP-источника

Copyright (c) 2026 bhgv

[English](README.md) · [Русский](README.ru.md) · [Examples](../README.ru.md)

## Что и зачем

Пример представляет ленивую иерархию как обычный сервер 9P: страны, города, районы, улицы, дома, комнаты и файлы info. HTTP-источник выдаёт только metadata одного ID или страницу непосредственных детей. Ни дерево целиком, ни все дочерние уровни не передаются и не фиксируются.

Исходный libstyx принимает Twalk/Topen/Tstat/Tread/Tclunk. C backend запускает Python helper без shell; тот запрашивает HTTP(S), проверяет весь JSON и только потом выдаёт строки в частную трубу. Эта труба — локальная деталь примера, не замена 9P или Unibus. Библиотека не получает зависимость от HTTP или Python.

Fixture создаёт синтетическую иерархию с миллиардами возможных объектов без хранения всего дерева. Это не реальные сведения о странах. Вместо него можно подключить интернет-сервис или БД, соблюдая схему ниже.

## Сборка и запуск

```sh
cmake -S . -B build -DSTYX_HOSTED=ON -DBUILD_TESTING=ON
cmake --build build -j
python3 examples/http_source.py 8080
```

Из корня пакета; fixture остаётся в первом терминале. Во втором запустите сервер, в третьем клиент:

```sh
./build/examples/styx-http 5640 http://127.0.0.1:8080/node
./build/examples/styx-cat 5640 Exampleland/city-2/district-3/street-4/house-5/room-6/info
```

Ожидается `Synthetic information for room-6` без LF. Выберите свободные порты; stdout каждого сервера первой строкой сообщает фактический порт. Завершение — Ctrl-C; отдельный helper сам заканчивается после запроса. Hosted пример требует Linux64/libc/pthread и Python3 stdlib, GUI/VM не нужны.

## Параметры и лимиты

| Параметр | Значение и смысл |
| --- | --- |
| styx-http argv1 | Decimal порт 0..65535; default 0 выбирает свободный |
| styx-http argv2 | Обязательный HTTP(S) endpoint, заданный оператором |
| Endpoint query | Дополнительные параметры сохраняются; op/id/start/limit и fragment запрещены |
| Helper args | endpoint, stat/list, unsigned64 id, unsigned64 start, limit 1..16 |
| HTTP timeout | 3с на операцию; process alarm 5с ограничивает весь helper |
| Response body | Максимум 128KiB; UTF-8 JSON array |
| Page size | До 16 непосредственных детей; stat — максимум одна строка |
| name / value | До 127 / 2047 UTF-8 байтов без NUL; value по умолчанию пуст |

## HTTP schema

```text
GET /node?op=stat&id=0&start=0&limit=1
GET /node?op=list&id=0&start=0&limit=16
```

```json
[{"id": 9007199254740993, "parent": 0, "mode": 292, "name": "large", "value": "text"}]
```

JSON numbers id/parent — точные целые 0..2⁶⁴−1: не float, string или bool. Python сохраняет их точно, но JavaScript backend должен избегать округления через Number. mode — целое из directory bit 0x80000000 и rwx bits 0777; 292 в примере — 0444.

Каждая строка — object с обязательными id/parent/mode/name; value необязателен и должен быть string. Неизвестные дополнительные поля игнорируются. Повторные JSON keys, NaN/Infinity, неверный UTF-8, NUL и превышение лимитов дают ошибку. Объявленный Content-Length должен быть единственным, ограниченным и полностью полученным; chunked+Content-Length отклоняется как неоднозначность. Усечённый HTTP body, даже содержащий [] целиком, не считается отсутствием. Корень имеет id=0, parent=0, name="/" и directory bit. Иное имя не пусто, не содержит / и не равно . или ..; ненулевой ID не может быть своим parent.

Для stat ID строки обязан совпадать с запрошенным; пустой массив означает отсутствие. Для list parent каждой строки обязан совпадать с запрошенным ID, сам каталог не может быть своим ребёнком. Внутри страницы имена и ID уникальны. Проверка не скачивает другие страницы для глобальной проверки дубликатов: source обязан сохранять согласованные ID и порядок между страницами.

## Изменения и открытые объекты

start — индекс строки, не byte offset 9P. C callbacks преобразуют directory offsets в encoded Dir records. Twalk разрешает компоненты, Topen проверяет mode; отдельного Twalk перед каждым read/write нет. Последующие stat/read запрашивают metadata по удержанному ID. Переименование того же ID не меняет объект открытого fid; данные читаются заново и могут измениться.

Этот HTTP backend не удерживает удалённую запись lease/refcount: если source удалил ID, следующий stat/read получит ошибку. Source не должен назначать тот же ID другому объекту. Изменения списка между страницами могут требовать rewind offset0; пример не обещает snapshot consistency. Большие данные потребуют собственного byte-stream read вместо ограниченного text value.

## Ошибки и блокирование

Helper проверяет весь ответ до stdout: неверная последняя строка не публикует правильный prefix. Непустой неверный ответ не превращается в []/EOF. Nonzero exit, timeout или transport failure дают backend error; 9P callback возвращает Rerror, исправное соединение остаётся доступным для следующих запросов. C catalog_query может иметь временные данные в output при неуспехе: caller обязан проверять success и не использовать их.

Проекция только для чтения: catalog_write всегда отказывает, HTTP mutation/retry не выполняется. HTTP backend синхронный: его server owner ждёт helper. Это учебный пример, не асинхронный многоклиентный HTTP scheduler. Для production slow backend используйте отдельного owner/worker и deferred-read API; codec не требует остановки VM или глобальной блокировки исполнения. Политика доступа и стабильность ID остаются у источника.

## Проверки и исходники

```sh
ctest --test-dir build -R '^styx-http' --output-on-failure
```

Базовый styx-http проходит глубокую lazy-иерархию и страницы по 16 строк. styx-http-contract проверяет schema, дубликаты, чужие ID/parents, UTF-8 byte limits, отсутствие частичного stdout и настоящее 9P-чтение ID 2⁵³+1 и 2⁶⁴−1. Mutable fixture меняет имена и данные между запросами; после Rerror проверяется восстановление обычного чтения по тому же соединению.

[HTTP validation/helper](../http_query.py) · [C backend](../http_catalog.c) · [Synthetic source](../http_source.py) · [Server callbacks](../server.c) · [Backend contract](../catalog.h) · [Contract tests](../../tests/http_contract.py)

[styxinit](../../docs/reference/styxinit.ru.md) · [styxprocess](../../docs/reference/styxprocess.ru.md) · [Main-project Styx help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
