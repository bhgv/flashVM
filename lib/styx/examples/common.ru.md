# Общий сервер: от запроса до backend и обратно

Copyright (c) 2026 bhgv

[English](common.md) · [Русский](common.ru.md) · [Examples](README.ru.md)

## Назначение

Пять моделей каталогов используют один C-сервер: fixed (flat/tree), database, live и HTTP. Они меняют только backend за private contract catalog.h. Это runnable teaching adapters, не отдельные реализации протокола: wire messages, fids и их retirement обслуживает исходная libstyx.

Все библиотеки и общий сервер написаны на C. Python wire_client/check — независимые тестовые клиенты; HTTP helper и fixture — части HTTP-примера. Они не превращают C codec, libstyx или libunibus в Python/C++ библиотеки.

## Карта исходников

| File | Responsibility |
| --- | --- |
| [server.c](server.c) | Original Styxops callbacks, byte offsets, metadata packing |
| [catalog.h](catalog.h) | Private backend inputs, result validity, open/close contract |
| [fixed.c](fixed.c) | Immutable flat/tree selection through CATALOG_TREE |
| [database.c](database.c) | Fresh bounded TSV query, logical key identities |
| [live.c](live.c) | Published names versus retained opened objects |
| [http_catalog.c](http_catalog.c) | Owned helper process and private pipe |
| [client.c](client.c) | Native retained-fid reader, styx-cat |
| [wire_client.py](wire_client.py) | Independent standard 9P test client |
| [check.py](check.py) | Process ownership and five model acceptance cases |


## Последовательность запросов

1. `main` в [server.c](server.c) вызывает catalog_init до styxinit. Fixed/live не требуют source; database/HTTP заимствуют argv string до завершения. Backend не строит remote tree на старте.
2. Twalk вызывает [walk_node](server.c#L58) для компонента: проверить текущий directory ID, найти ребёнка среди immediate pages или разрешить `..`. Менять Qid только после успеха. Twalk не выполняется повторно перед каждым read/write.
3. Tstat вызывает [stat_node](server.c#L108); [attributes](server.c#L36) строит Dir с borrowed strings. Dispatcher упаковывает их до следующего callback. Metadata buffer общий, поэтому callbacks выполняет один server owner.
4. Topen вызывает [open_node](server.c#L128): проверить текущий mode, разрешить exact OREAD/OWRITE и получить один pin при успехе. Нет неявного create/truncate. Root/metadata не выдают дополнительных credentials; mode здесь demonstration policy.
5. Tread вызывает [read_node](server.c#L169) по удержанному Qid и byte offset. Для файла — текущий текст; для каталога — целые Dir непосредственных детей. Весь namespace не сериализуется.
6. Twrite вызывает [write_node](server.c#L235) один раз и сохраняет accepted byte count. В этих backend только live принимает ctl-команды; другие возвращают отказ. Short write не заменяется full acknowledgement и не повторяется.
7. Tclunk или disconnect вызывает [close_node](server.c#L149) для успешного open. Unpin идёт по исходному ID, без name lookup/reopen. После SIGINT/SIGTERM main выходит из цикла и styxend завершает server resources.

## Страницы, offsets и ошибки

catalog_query получает start в строках и capacity в строках. На уровне 9P directory offset измеряется байтами encoded Dir. read_node проходит страницы и накапливает размеры записей, пока не дойдёт до offset. Запись не делится между двумя ответами; слишком маленький первый буфер даёт error, середина записи — сообщение о rewind. Изменение каталога между запросами может потребовать offset0; snapshot не обещается.

query success=1 с count=0 означает отсутствие/конец. success=0 означает ошибку; output/count тогда provisional и не используются. Stat/walk metadata failure отображается в Enonexist, listing failure — в backend query failed: оба идут как Rerror, не пустой успешный read. Буферы, частично заполненные перед error, нельзя считать подтверждёнными данными.

## Время жизни backend

Fixed rows живут весь процесс: pin/unpin не приобретают ресурсов. Live удерживает общее место directory/title/button; скрытие имени не удаляет opened object, final unpin освобождает место, ID не переиспользуется. Database/HTTP не получают record lease: открытый fid сохраняет logical ID, но следующие reads заново запрашивают источник и могут отказать после удаления. Имя и underlying logical identity — разные понятия.

TSV bounded до128 ключей; FNV64 — demonstration key ID, collisions/повторы внутри текущего набора дают отказ. Не используйте этот hash как access token. HTTP helper ограничен3с на операцию, alarm5с и body128KiB. Эти backend синхронны: server owner ждёт I/O. Пример не является production asynchronous scheduler; медленные источники требуют отдельного owner/worker и deferred API. VM/остальные исполняющиеся потоки глобально не останавливаются.

## Запуск нативного клиента

```sh
./build/examples/styx-flat 5640
```

В другом терминале:

```sh
./build/examples/styx-cat 5640 status
```

[client.c](client.c) принимает ровно PORT PATH, соединяется с loopback, передаёт owned descriptor в styxclientmountfd и далее владеет client/fid. Constructor поглощает descriptor и при отказе. Клиент делает walk/open, читает confirmed chunks и движется только на фактический count. Confirmed zero — EOF; error/uncertain — отказ, не retry. После чтения идут clunk, shutdown, free. Коды: 0 EOF, 1 arguments/socket, 2 mount, 3 walk/open/read/short fwrite. Это blocking text utility, не reconnecting/general shell client.

[styxinit](../docs/reference/styxinit.ru.md) · [styxprocess](../docs/reference/styxprocess.ru.md) · [styxend](../docs/reference/styxend.ru.md) · [styxclientmountfd](../docs/reference/styxclientmountfd.ru.md) · [styxclientread](../docs/reference/styxclientread.ru.md) · [convD2M](../docs/reference/convD2M.ru.md)
