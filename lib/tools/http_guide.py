# Copyright (c) 2026 bhgv
# EN: Authored HTTP catalog tutorial and response schema; no runtime generation.
# RU: Авторское HTTP catalog руководство и схема ответов; runtime не генерируется.
from pathlib import Path
from .introductions import page


# EN: Write EN/RU HTTP example guides beneath borrowed Styx package root.
# Keep request limits and supported behavior explicit; source paths stay local.
# RU: Записать EN/RU HTTP guides под root пакета Styx. Явно описать лимиты
# и поддержанное поведение; source links остаются внутри копируемого пакета.
def main(root):
    for ru in (False, True):
        suffix = '.ru.md' if ru else '.md'
        text = '# ' + ('Динамическое пространство имён из HTTP-источника' if ru else 'Dynamic namespace from an HTTP source')
        text += f'\n\nCopyright (c) 2026 bhgv\n\n[English](README.md) · [Русский](README.ru.md) · [Examples](../README{suffix})\n\n'
        text += '''## Что и зачем

Пример представляет ленивую иерархию как обычный сервер 9P: страны, города, районы, улицы, дома, комнаты и файлы info. HTTP-источник выдаёт только metadata одного ID или страницу непосредственных детей. Ни дерево целиком, ни все дочерние уровни не передаются и не фиксируются.

Исходный libstyx принимает Twalk/Topen/Tstat/Tread/Tclunk. C backend запускает Python helper без shell; тот запрашивает HTTP(S), проверяет весь JSON и только потом выдаёт строки в частную трубу. Эта труба — локальная деталь примера, не замена 9P или Unibus. Библиотека не получает зависимость от HTTP или Python.

Fixture создаёт синтетическую иерархию с миллиардами возможных объектов без хранения всего дерева. Это не реальные сведения о странах. Вместо него можно подключить интернет-сервис или БД, соблюдая схему ниже.
''' if ru else '''## What and why

This example exposes a lazy hierarchy as an ordinary 9P server: countries, cities, districts, streets, houses, rooms and info files. The HTTP source returns metadata for one ID or one page of immediate children. It never transmits or freezes a complete tree or every descendant level.

The original libstyx handles Twalk/Topen/Tstat/Tread/Tclunk. The C backend starts a Python helper without a shell; it requests HTTP(S), validates the entire JSON body and only then writes rows to a private pipe. That pipe is a local example detail, not a replacement for 9P or Unibus. The library acquires no HTTP or Python dependency.

The fixture computes a synthetic hierarchy with billions of possible objects without storing the tree. It is not real geographic information. An internet service or database can replace it using the schema below.
'''
        text += '\n## ' + ('Сборка и запуск' if ru else 'Build and run') + '''

```sh
cmake -S . -B build -DSTYX_HOSTED=ON -DBUILD_TESTING=ON
cmake --build build -j
python3 examples/http_source.py 8080
```

'''
        text += ('Из корня пакета; fixture остаётся в первом терминале. Во втором запустите сервер, в третьем клиент:\n' if ru else 'From the package root; leave the fixture in the first terminal. Start the server in a second terminal and the client in a third:\n')
        text += '''
```sh
./build/examples/styx-http 5640 http://127.0.0.1:8080/node
./build/examples/styx-cat 5640 Exampleland/city-2/district-3/street-4/house-5/room-6/info
```

'''
        text += ('Ожидается `Synthetic information for room-6` без LF. Выберите свободные порты; stdout каждого сервера первой строкой сообщает фактический порт. Завершение — Ctrl-C; отдельный helper сам заканчивается после запроса. Hosted пример требует Linux64/libc/pthread и Python3 stdlib, GUI/VM не нужны.\n' if ru else 'Expect `Synthetic information for room-6` without LF. Choose available ports; each server announces the actual port in its first stdout line. Stop them with Ctrl-C; each helper exits after its request. The hosted example requires Linux64/libc/pthread and Python3 stdlib, without GUI or VM.\n')
        text += '\n## ' + ('Параметры и лимиты' if ru else 'Parameters and limits') + '\n\n'
        if ru:
            text += '''| Параметр | Значение и смысл |
| --- | --- |
| styx-http argv1 | Decimal порт 0..65535; default 0 выбирает свободный |
| styx-http argv2 | Обязательный HTTP(S) endpoint, заданный оператором |
| Endpoint query | Дополнительные параметры сохраняются; op/id/start/limit и fragment запрещены |
| Helper args | endpoint, stat/list, unsigned64 id, unsigned64 start, limit 1..16 |
| HTTP timeout | 3с на операцию; process alarm 5с ограничивает весь helper |
| Response body | Максимум 128KiB; UTF-8 JSON array |
| Page size | До 16 непосредственных детей; stat — максимум одна строка |
| name / value | До 127 / 2047 UTF-8 байтов без NUL; value по умолчанию пуст |
'''
        else:
            text += '''| Parameter | Value and effect |
| --- | --- |
| styx-http argv1 | Decimal port 0..65535; default 0 selects an available port |
| styx-http argv2 | Required operator-supplied HTTP(S) endpoint |
| Endpoint query | Other parameters are preserved; op/id/start/limit and fragments are rejected |
| Helper args | endpoint, stat/list, unsigned64 id, unsigned64 start, limit 1..16 |
| HTTP timeout | 3s per operation; a 5s process alarm bounds the entire helper |
| Response body | At most 128KiB; UTF-8 JSON array |
| Page size | Up to 16 immediate children; stat returns at most one row |
| name / value | Up to 127 / 2047 UTF-8 bytes without NUL; value defaults to empty |
'''
        text += '\n## HTTP schema\n\n```text\nGET /node?op=stat&id=0&start=0&limit=1\nGET /node?op=list&id=0&start=0&limit=16\n```\n\n```json\n[{"id": 9007199254740993, "parent": 0, "mode": 292, "name": "large", "value": "text"}]\n```\n\n'
        text += ('JSON numbers id/parent — точные целые 0..2⁶⁴−1: не float, string или bool. Python сохраняет их точно, но JavaScript backend должен избегать округления через Number. mode — целое из directory bit 0x80000000 и rwx bits 0777; 292 в примере — 0444.\n\nКаждая строка — object с обязательными id/parent/mode/name; value необязателен и должен быть string. Неизвестные дополнительные поля игнорируются. Повторные JSON keys, NaN/Infinity, неверный UTF-8, NUL и превышение лимитов дают ошибку. Объявленный Content-Length должен быть единственным, ограниченным и полностью полученным; chunked+Content-Length отклоняется как неоднозначность. Усечённый HTTP body, даже содержащий [] целиком, не считается отсутствием. Корень имеет id=0, parent=0, name="/" и directory bit. Иное имя не пусто, не содержит / и не равно . или ..; ненулевой ID не может быть своим parent.\n\nДля stat ID строки обязан совпадать с запрошенным; пустой массив означает отсутствие. Для list parent каждой строки обязан совпадать с запрошенным ID, сам каталог не может быть своим ребёнком. Внутри страницы имена и ID уникальны. Проверка не скачивает другие страницы для глобальной проверки дубликатов: source обязан сохранять согласованные ID и порядок между страницами.\n' if ru else 'JSON id/parent numbers are exact integers in 0..2⁶⁴−1: not floats, strings or booleans. Python preserves them exactly; a JavaScript backend must avoid rounding through Number. mode combines directory bit 0x80000000 and rwx bits 0777; 292 above means 0444.\n\nEach row is an object with required id/parent/mode/name; optional value must be a string. Unknown extra fields are ignored. Duplicate JSON keys, NaN/Infinity, invalid UTF-8, NUL and exceeded limits fail. A declared Content-Length must be unique, bounded and fully received; chunked+Content-Length is rejected as ambiguous. A truncated HTTP body, even containing a complete [], never means absence. The root has id=0, parent=0, name="/" and the directory bit. Other names are nonempty, contain no / and are neither . nor ..; a nonzero ID cannot be its own parent.\n\nFor stat, the returned row ID must equal the requested ID; an empty array means absence. For list, every parent must equal the requested ID, and the directory cannot be its own child. Names and IDs are unique within each page. Validation never fetches other pages to check global duplicates: the source must preserve consistent IDs and ordering across pages.\n')
        text += '\n## ' + ('Изменения и открытые объекты' if ru else 'Changes and opened objects') + '\n\n'
        text += ('start — индекс строки, не byte offset 9P. C callbacks преобразуют directory offsets в encoded Dir records. Twalk разрешает компоненты, Topen проверяет mode; отдельного Twalk перед каждым read/write нет. Последующие stat/read запрашивают metadata по удержанному ID. Переименование того же ID не меняет объект открытого fid; данные читаются заново и могут измениться.\n\nЭтот HTTP backend не удерживает удалённую запись lease/refcount: если source удалил ID, следующий stat/read получит ошибку. Source не должен назначать тот же ID другому объекту. Изменения списка между страницами могут требовать rewind offset0; пример не обещает snapshot consistency. Большие данные потребуют собственного byte-stream read вместо ограниченного text value.\n' if ru else 'start is a row index, not a 9P byte offset. C callbacks convert directory offsets to encoded Dir records. Twalk resolves components and Topen checks mode; there is no extra Twalk before every read/write. Later stat/read requests fetch metadata by the retained ID. Renaming that ID does not change the object of an opened fid; content is fetched again and may change.\n\nThis HTTP backend does not retain the remote record through a lease/refcount: if the source deletes an ID, the next stat/read fails. The source must never reassign that ID to another object. Listings changed between pages may require rewind offset0; this example promises no snapshot consistency. Large data needs a byte-stream read implementation instead of bounded text value.\n')
        text += '\n## ' + ('Ошибки и блокирование' if ru else 'Errors and blocking') + '\n\n'
        text += ('Helper проверяет весь ответ до stdout: неверная последняя строка не публикует правильный prefix. Непустой неверный ответ не превращается в []/EOF. Nonzero exit, timeout или transport failure дают backend error; 9P callback возвращает Rerror, исправное соединение остаётся доступным для следующих запросов. C catalog_query может иметь временные данные в output при неуспехе: caller обязан проверять success и не использовать их.\n\nПроекция только для чтения: catalog_write всегда отказывает, HTTP mutation/retry не выполняется. HTTP backend синхронный: его server owner ждёт helper. Это учебный пример, не асинхронный многоклиентный HTTP scheduler. Для production slow backend используйте отдельного owner/worker и deferred-read API; codec не требует остановки VM или глобальной блокировки исполнения. Политика доступа и стабильность ID остаются у источника.\n' if ru else 'The helper validates the entire response before stdout: an invalid final row never publishes a valid prefix. An invalid nonempty response never becomes []/EOF. Nonzero exit, timeout or transport failure gives a backend error; the 9P callback returns Rerror and a healthy connection remains available for subsequent requests. C catalog_query may contain provisional output on failure: callers must check success and never consume it.\n\nThe projection is readonly: catalog_write always refuses and performs no HTTP mutation/retry. The HTTP backend is synchronous: its server owner waits for the helper. This is a teaching example, not an asynchronous multi-client HTTP scheduler. A production slow backend needs a separate owner/worker and deferred-read API; the codec requires no VM stop or global execution lock. Access policy and stable IDs remain the source’s responsibility.\n')
        text += '\n## ' + ('Проверки и исходники' if ru else 'Checks and sources') + '''

```sh
ctest --test-dir build -R '^styx-http' --output-on-failure
```

'''
        text += ('Базовый styx-http проходит глубокую lazy-иерархию и страницы по 16 строк. styx-http-contract проверяет schema, дубликаты, чужие ID/parents, UTF-8 byte limits, отсутствие частичного stdout и настоящее 9P-чтение ID 2⁵³+1 и 2⁶⁴−1. Mutable fixture меняет имена и данные между запросами; после Rerror проверяется восстановление обычного чтения по тому же соединению.\n' if ru else 'The base styx-http test walks a deep lazy hierarchy with 16-row pages. styx-http-contract checks schema, duplicates, foreign IDs/parents, UTF-8 byte limits, absence of partial stdout and real 9P reads of IDs 2⁵³+1 and 2⁶⁴−1. A mutable fixture changes names and content between requests; ordinary reads on the same connection must recover after Rerror.\n')
        text += f'\n[HTTP validation/helper](../http_query.py) · [C backend](../http_catalog.c) · [Synthetic source](../http_source.py) · [Server callbacks](../server.c) · [Backend contract](../catalog.h) · [Contract tests](../../tests/http_contract.py)\n\n[styxinit](../../docs/reference/styxinit{suffix}) · [styxprocess](../../docs/reference/styxprocess{suffix}) · [Main-project Styx help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)\n'
        page(root, 'examples/http/README' + suffix, text)


if __name__ == '__main__':
    main(Path(__file__).resolve().parents[1] / 'styx')
