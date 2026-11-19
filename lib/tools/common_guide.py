# Copyright (c) 2026 bhgv
# EN: Authored EN/RU walkthrough of the shared example callbacks and lifetimes.
# RU: Авторский EN/RU разбор общих callbacks примеров и времени жизни ресурсов.
from pathlib import Path
from .introductions import page
from .memory_guide import source_link


# EN: Write two walkthroughs beneath borrowed Styx root. Source anchors are resolved
# from actual definitions; no runtime code or API contract is inferred/generated.
# RU: Записать два walkthrough под Styx root. Source anchors берутся из настоящих
# definitions; runtime code и API contracts не выводятся из синтаксиса.
def main(root):
    base = root / 'examples'
    links = {name: source_link(base, 'server.c', name) for name in
             ('attributes', 'walk_node', 'stat_node', 'open_node', 'close_node', 'read_node', 'write_node')}
    for ru in (False, True):
        suffix = '.ru.md' if ru else '.md'
        title = 'Общий сервер: от запроса до backend и обратно' if ru else 'Shared server: request to backend and back'
        text = f'# {title}\n\nCopyright (c) 2026 bhgv\n\n[English](common.md) · [Русский](common.ru.md) · [Examples](README{suffix})\n\n'
        text += '''## Назначение

Пять моделей каталогов используют один C-сервер: fixed (flat/tree), database, live и HTTP. Они меняют только backend за private contract catalog.h. Это runnable teaching adapters, не отдельные реализации протокола: wire messages, fids и их retirement обслуживает исходная libstyx.

Все библиотеки и общий сервер написаны на C. Python wire_client/check — независимые тестовые клиенты; HTTP helper и fixture — части HTTP-примера. Они не превращают C codec, libstyx или libunibus в Python/C++ библиотеки.
''' if ru else '''## Purpose

Five catalog models share one C server: fixed (flat/tree), database, live and HTTP. Only the backend behind the private catalog.h contract changes. These are runnable teaching adapters, not separate protocol implementations: the original libstyx handles wire messages, fids and their retirement.

The libraries and shared server are implemented in C. Python wire_client/check are independent test clients; the HTTP helper and fixture belong to the HTTP example. They do not turn the C codec, libstyx or libunibus into Python/C++ libraries.
'''
        text += '\n## ' + ('Карта исходников' if ru else 'Source map') + '\n\n'
        text += '''| File | Responsibility |
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

'''
        text += '\n## ' + ('Последовательность запросов' if ru else 'Request flow') + '\n\n'
        if ru:
            text += f'''1. `main` в [server.c](server.c) вызывает catalog_init до styxinit. Fixed/live не требуют source; database/HTTP заимствуют argv string до завершения. Backend не строит remote tree на старте.
2. Twalk вызывает {links['walk_node']} для компонента: проверить текущий directory ID, найти ребёнка среди immediate pages или разрешить `..`. Менять Qid только после успеха. Twalk не выполняется повторно перед каждым read/write.
3. Tstat вызывает {links['stat_node']}; {links['attributes']} строит Dir с borrowed strings. Dispatcher упаковывает их до следующего callback. Metadata buffer общий, поэтому callbacks выполняет один server owner.
4. Topen вызывает {links['open_node']}: проверить текущий mode, разрешить exact OREAD/OWRITE и получить один pin при успехе. Нет неявного create/truncate. Root/metadata не выдают дополнительных credentials; mode здесь demonstration policy.
5. Tread вызывает {links['read_node']} по удержанному Qid и byte offset. Для файла — текущий текст; для каталога — целые Dir непосредственных детей. Весь namespace не сериализуется.
6. Twrite вызывает {links['write_node']} один раз и сохраняет accepted byte count. В этих backend только live принимает ctl-команды; другие возвращают отказ. Short write не заменяется full acknowledgement и не повторяется.
7. Tclunk или disconnect вызывает {links['close_node']} для успешного open. Unpin идёт по исходному ID, без name lookup/reopen. После SIGINT/SIGTERM main выходит из цикла и styxend завершает server resources.
'''
        else:
            text += f'''1. `main` in [server.c](server.c) calls catalog_init before styxinit. Fixed/live need no source; database/HTTP borrow the argv string until exit. The backend does not construct a remote tree at startup.
2. Twalk invokes {links['walk_node']} for a component: validate the current directory ID, find an immediate child through pages, or resolve `..`. Qid changes only after success. No extra Twalk precedes every read/write.
3. Tstat invokes {links['stat_node']}; {links['attributes']} builds Dir with borrowed strings. The dispatcher packs them before the next callback. Metadata storage is shared, so callbacks run under one server owner.
4. Topen invokes {links['open_node']}: check current mode, permit exact OREAD/OWRITE and acquire one pin on success. There is no implicit create/truncate. Root/metadata grant no extra credentials; mode is demonstration policy here.
5. Tread invokes {links['read_node']} using the retained Qid and byte offset. Files yield current text; directories yield whole Dir records for immediate children. A whole namespace is never serialized.
6. Twrite invokes {links['write_node']} once and preserves the accepted byte count. Only live accepts ctl commands; the other backends refuse writes. A short write is never promoted to full acknowledgement or replayed.
7. Tclunk or disconnect invokes {links['close_node']} for a successful open. Unpin uses the original ID without a name lookup/reopen. SIGINT/SIGTERM makes main leave its loop, then styxend retires server resources.
'''
        text += '\n## ' + ('Страницы, offsets и ошибки' if ru else 'Pages, offsets and errors') + '\n\n'
        text += ('catalog_query получает start в строках и capacity в строках. На уровне 9P directory offset измеряется байтами encoded Dir. read_node проходит страницы и накапливает размеры записей, пока не дойдёт до offset. Запись не делится между двумя ответами; слишком маленький первый буфер даёт error, середина записи — сообщение о rewind. Изменение каталога между запросами может потребовать offset0; snapshot не обещается.\n\nquery success=1 с count=0 означает отсутствие/конец. success=0 означает ошибку; output/count тогда provisional и не используются. Stat/walk metadata failure отображается в Enonexist, listing failure — в backend query failed: оба идут как Rerror, не пустой успешный read. Буферы, частично заполненные перед error, нельзя считать подтверждёнными данными.\n' if ru else 'catalog_query takes start and capacity in rows. At the 9P layer, a directory offset counts bytes of encoded Dir records. read_node scans pages and accumulates record sizes until it reaches the offset. A record is never split across replies; an undersized first buffer fails, and an offset inside a record reports rewind. Changes between requests can require offset0; no snapshot is promised.\n\nquery success=1 with count=0 means absence/end. success=0 means failure; output/count are provisional and unused. Stat/walk metadata failure maps to Enonexist, listing failure to backend query failed: both are Rerror, not a successful empty read. Buffers partly filled before failure are not confirmed data.\n')
        text += '\n## ' + ('Время жизни backend' if ru else 'Backend lifetimes') + '\n\n'
        text += ('Fixed rows живут весь процесс: pin/unpin не приобретают ресурсов. Live удерживает общее место directory/title/button; скрытие имени не удаляет opened object, final unpin освобождает место, ID не переиспользуется. Database/HTTP не получают record lease: открытый fid сохраняет logical ID, но следующие reads заново запрашивают источник и могут отказать после удаления. Имя и underlying logical identity — разные понятия.\n\nTSV bounded до128 ключей; FNV64 — demonstration key ID, collisions/повторы внутри текущего набора дают отказ. Не используйте этот hash как access token. HTTP helper ограничен3с на операцию, alarm5с и body128KiB. Эти backend синхронны: server owner ждёт I/O. Пример не является production asynchronous scheduler; медленные источники требуют отдельного owner/worker и deferred API. VM/остальные исполняющиеся потоки глобально не останавливаются.\n' if ru else 'Fixed rows live for the entire process: pin/unpin acquire no resources. Live retains a shared directory/title/button slot; hiding a name does not delete an opened object, final unpin frees the slot, and IDs are never reused. Database/HTTP acquire no record lease: an opened fid retains a logical ID, but later reads query the source again and may fail after deletion. A name and its underlying logical identity are distinct.\n\nTSV is bounded to128 keys; FNV64 is a demonstration key ID and collisions/duplicates in the current set fail. Do not treat the hash as an access token. The HTTP helper uses a3s operation timeout,5s alarm and128KiB body limit. These backends are synchronous: the server owner waits for I/O. This is not a production asynchronous scheduler; slow sources need a separate owner/worker and deferred API. No global VM/execution-thread stop is required.\n')
        text += '\n## ' + ('Запуск нативного клиента' if ru else 'Running the native client') + '''

```sh
./build/examples/styx-flat 5640
```

'''
        text += ('В другом терминале:\n' if ru else 'In another terminal:\n') + '''
```sh
./build/examples/styx-cat 5640 status
```

'''
        text += ('[client.c](client.c) принимает ровно PORT PATH, соединяется с loopback, передаёт owned descriptor в styxclientmountfd и далее владеет client/fid. Constructor поглощает descriptor и при отказе. Клиент делает walk/open, читает confirmed chunks и движется только на фактический count. Confirmed zero — EOF; error/uncertain — отказ, не retry. После чтения идут clunk, shutdown, free. Коды: 0 EOF, 1 arguments/socket, 2 mount, 3 walk/open/read/short fwrite. Это blocking text utility, не reconnecting/general shell client.\n' if ru else '[client.c](client.c) takes exactly PORT PATH, connects to loopback, transfers its owned descriptor to styxclientmountfd and then owns client/fid. The constructor consumes the descriptor on failure too. The client walks/opens, reads confirmed chunks and advances only by actual count. Confirmed zero is EOF; error/uncertain means failure, never replay. It then clunks, shuts down and frees. Exit codes:0 EOF,1 arguments/socket,2 mount,3 walk/open/read/short fwrite. This is a blocking text utility, not a reconnecting/general shell client.\n')
        text += f'\n[styxinit](../docs/reference/styxinit{suffix}) · [styxprocess](../docs/reference/styxprocess{suffix}) · [styxend](../docs/reference/styxend{suffix}) · [styxclientmountfd](../docs/reference/styxclientmountfd{suffix}) · [styxclientread](../docs/reference/styxclientread{suffix}) · [convD2M](../docs/reference/convD2M{suffix})\n'
        page(root, 'examples/common' + suffix, text)


if __name__ == '__main__':
    main(Path(__file__).resolve().parents[1] / 'styx')
