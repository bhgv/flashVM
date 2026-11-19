# Copyright (c) 2026 bhgv
# EN: Author-maintained EN/RU example recipes and protocol/porting guides, separate from extracted API metadata.
# RU: Авторские EN/RU рецепты примеров и руководства протокола/переноса отдельно от извлечённых metadata API.
from pathlib import Path

MODELS = {
    "flat": ("Fixed root files", "Фиксированные файлы корня", "fixed.c", "status; temperature", "status", "ready\\n",
             "The two names and contents are constant. No backend enumeration occurs beyond the requested root. This is the smallest sensor-style server; replace the temperature read with your device sampling callback.",
             "Два имени и содержимое постоянны. Backend не перечисляет ничего, кроме запрошенного корня. Это минимальная модель датчика: чтение temperature заменяется callback измерения устройства."),
    "tree": ("Fixed three-level namespace", "Фиксированное трёхуровневое пространство", "fixed.c", "devices/sensor/temperature; system/info", "devices/sensor/temperature", "21.5\\n",
             "The same fixed backend is compiled with CATALOG_TREE. Directory components change the current fid through ordinary Twalk. Reading a directory returns metadata for immediate children, not a serialized recursive tree.",
             "Тот же fixed backend собирается с CATALOG_TREE. Компоненты каталогов меняют fid обычным Twalk. Чтение каталога возвращает metadata непосредственных детей, не сериализованное рекурсивное дерево."),
    "database": ("Live key/value database view", "Живое представление БД ключ/значение", "database.c", "each current key is one file", "alpha", "one",
             "A fresh explicitly supplied TSV source is opened for each query. A key is a filename, its value is file content. Updating/replacing the source changes later walks, directory reads and value reads without restarting the server. FNV64 IDs are demo identities; collisions and duplicate keys are rejected, not silently aliased. A real SQL backend should use stable primary keys and parameterized queries.",
             "При каждом запросе заново читается явно заданный TSV-источник. Ключ — имя файла, значение — содержимое. Изменение/замена источника меняет последующие обходы, каталоги и чтения без перезапуска сервера. FNV64 здесь demonstration-ID; коллизии и повторы отклоняются. Для SQL нужны стабильные primary keys и параметризованные запросы."),
    "http": ("Lazy paginated HTTP hierarchy", "Ленивая постраничная HTTP-иерархия", "http_catalog.c", "country/city/district/street/house/room/info", "Exampleland/city-2/district-3/street-4/house-5/room-6/info", "Synthetic information for room-6",
             "Every stat/list query runs the HTTP adapter with the selected stable object ID and one immediate-child page. Startup does not fetch the tree. The supplied real HTTP fixture computes a synthetic hierarchy with billions of nodes without storing it. It is a reproducible test, not real geographic information. Replace the JSON service with an Internet/database adapter for your actual data.",
             "Каждый stat/list вызывает HTTP-adapter с выбранным стабильным ID и одной страницей непосредственных детей. На старте дерево не загружается. Настоящий HTTP-fixture вычисляет условную иерархию с миллиардами узлов без её хранения. Это воспроизводимый тест, не реальные географические сведения. JSON-сервис заменяется adapter настоящего интернет/БД источника."),
    "live": ("Live window/control registry", "Живой реестр окон/контролов", "live.c", "ctl; window-ID/title; window-ID/button", "window-100/title", "Live window",
             "The backend maintains a small native registry, independent of any GUI toolkit. Writing add/remove commands to ctl publishes/hides entries immediately. Stable IDs are never reused. Hiding a name does not retire an already opened object: catalog_pin/catalog_unpin keep it alive through the original server's open/close callbacks. Adapt these queries to your application's real window/control registry.",
             "Backend содержит небольшой нативный реестр без GUI toolkit. Запись add/remove в ctl сразу публикует/скрывает записи. Стабильные ID не переиспользуются. Скрытие имени не удаляет открытый объект: catalog_pin/catalog_unpin удерживают его через open/close исходного сервера. Запросы заменяются обращениями к реальному реестру окон/контролов приложения."),
}


# EN/RU: Write one owned UTF-8 guide and create only its parent directories; path/content are borrowed.
# Записать UTF-8 руководство, создав только родительские каталоги; path/content заимствованы.
def write(path, content):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content.strip() + "\n")


# EN/RU: Generate detailed language-separated recipes with actual executable names, option ranges and source/help links.
# Сформировать подробные раздельные языковые рецепты с настоящими executable names, диапазонами и source/help links.
def examples(root):
    for language in (0, 1):
        suffix = ".ru.md" if language else ".md"
        index = ["# " + ("Примеры Styx" if language else "Styx examples"), "", "Copyright (c) 2026 bhgv", "",
                 f"[Guide](../README{suffix}) · [API](../docs/index{suffix})", ""]
        for key, values in MODELS.items():
            en, ru, source, names, path, expected, why_en, why_ru = values
            title = ru if language else en
            argument = " keys.tsv" if key == "database" else " http://127.0.0.1:8080/node" if key == "http" else ""
            setup = ""
            if key == "database":
                setup = "printf 'alpha\\tone\\nbeta\\ttwo\\n' > keys.tsv  # Explicit demo database / Явная demo-БД\n"
            if key == "http":
                setup = "python3 examples/http_source.py 8080  # Run in another terminal / Запустить в другом терминале\n"
            content = f"""# {title}

Copyright (c) 2026 bhgv

[{'English' if language else 'Русский'}](README{'.md' if language else '.ru.md'}) · [Index](../README{suffix})

## {'Назначение и выбор модели' if language else 'Purpose and choice'}

{why_ru if language else why_en}

## {'Сборка и запуск' if language else 'Build and run'}

```sh
cmake -S . -B build -DSTYX_HOSTED=ON -DBUILD_TESTING=ON  # Hosted example targets / Нативные примеры
cmake --build build -j  # Build the library and examples / Собрать библиотеку и примеры
{setup}./build/examples/styx-{key} 5640{argument}  # Keep running in terminal1 / Оставить в терминале1
```

```sh
./build/examples/styx-cat 5640 {path}  # Read through original native 9P client / Читать нативным 9P-клиентом
ctest --test-dir build -R '^styx-{key}$' --output-on-failure  # Independent wire acceptance / Независимая wire-проверка
```

## {'Параметры' if language else 'Parameters'}

| {'Параметр' if language else 'Parameter'} | {'Варианты' if language else 'Values'} | {'Смысл' if language else 'Meaning'} |
| --- | --- | --- |
| argv1 | `0..65535`, default `0` | {'TCP-порт;0 выбирает свободный. Первая stdout-строка — реальный порт.' if language else 'TCP port;0 chooses an ephemeral port. First stdout line is the actual port.'} |
| argv2 | {'TSV path' if key == 'database' else 'HTTP(S) endpoint URL' if key == 'http' else 'unused'} | {'Источник явно задаёт оператор; клиент не передаёт host-пути.' if language else 'Operator supplies the source explicitly; clients do not supply host paths.'} |
| names | `{names}` | {'Пространство имён этого примера.' if language else 'Namespace of this example.'} |
| file sample | `{path}` | {'Ожидаемый текст: ' if language else 'Expected text: '}`{expected}` |

## {'Как проходят запросы' if language else 'Request flow'}

{'Twalk разрешает один или несколько компонентов в fid. Topen проверяет режим и удерживает объект. Tread файла использует удерживаемый ID и offset. Tread каталога кодирует только целые Dir непосредственных детей; offset измеряется байтами. Tclunk/разрыв вызывает cleanup. Между запросами дерево может меняться; повторный обход перед каждым read/write не добавляется.' if language else 'Twalk resolves components into a fid. Topen validates mode and pins the object. File Tread uses its held ID and offset. Directory Tread emits only complete Dir entries for immediate children; offset counts encoded bytes. Tclunk/disconnect performs cleanup. The tree may change between requests; no walk is added before every read/write.'}

## {'Границы и изменение backend' if language else 'Bounds and replacing the backend'}

{'Общий CatalogRow содержит имя до127 UTF-8 байт и demonstration text до2047 байт; страница содержит максимум16 детей. Для фотографий/больших данных заменить текстовый read на byte-stream/read по offset. Это не требование 9P хранить файл целиком. Списки, изменившиеся между страницами, могут требовать rewind offset0. Backend-вызовы примера синхронны; production asynchronous I/O использует deferred-read API.' if language else 'CatalogRow holds a name up to127 UTF-8 bytes and demo text up to2047 bytes; a page has at most16 children. For photos/large content replace text reads with byte-stream/offset reads. 9P does not require a whole file in memory. A listing changed between pages can require rewind to offset0. Example backend calls are synchronous; production asynchronous I/O uses the deferred-read API.'}

{'TSV-пример ограничен128 ключами; он перечитывает один плоский набор, не рекурсивное дерево. Live-пример имеет16 мест для видимых/удерживаемых окон. HTTP-helper ограничивает ответ128KiB, socket timeout3с и process alarm5с; IDs должны точно сохранять uint64, без округления через JavaScript double.' if language else 'The TSV demo is bounded to128 keys and rereads one flat dataset, not a recursive tree. The live demo has16 visible/held window slots. The HTTP helper bounds responses to128KiB, socket waits to3s and uses a5s process alarm; IDs must preserve exact uint64 values, without JavaScript-double rounding.'}

## {'Исходники и справка' if language else 'Source and reference'}

- [Server callbacks](../server.c), [catalog_query / pin / unpin contract](../catalog.h), [backend implementation](../{source}).
- [styxinit](../../docs/reference/styxinit{suffix}), [convD2M](../../docs/reference/convD2M{suffix}), [convM2S](../../docs/reference/convM2S{suffix}).
- [Original client](../client.c), [independent wire client](../wire_client.py), [acceptance](../check.py).
- [Main-project native Styx help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md).
"""
            if key == "live":
                content += "\n## " + ("Изменение реестра" if language else "Changing the registry") + "\n\n```python\nfrom wire_client import Client  # PYTHONPATH=examples; standard 9P client / Стандартный клиент\nc = Client(5640)  # Own one connection / Владеть соединением\nc.write('ctl', 'add')  # Publish a new window / Опубликовать новое окно\nprint(c.directory('/'))  # Observe current names / Увидеть текущие имена\nc.write('ctl', 'remove 100')  # Hide name, retain open objects / Скрыть имя, удержать open-объекты\nc.close()  # Retire fids and connection / Освободить fid и соединение\n```\n"
            if key == "http":
                content += "\n## HTTP backend schema\n\n`GET endpoint?op=list&id=0&start=0&limit=16` returns a JSON array. `op=stat` returns zero or one row. Each row has exact unsigned64 `id`/`parent`, numeric standard `mode`, UTF-8 `name`, optional text `value`. The source owns stable identities and access policy; changing a filename must not alias an unrelated held ID.\n\n[HTTP adapter](../http_query.py) · [HTTP fixture](../http_source.py).\n"
            write(root / f"styx/examples/{key}/README{suffix}", content)
            index.append(f"- [{title}]({key}/README{suffix}) — `styx-{key}`")
        index.extend(["", "[Core guide](../README" + suffix + ") · [Native client source](client.c) · [Backend contract](catalog.h)", ""])
        write(root / f"styx/examples/README{suffix}", "\n".join(index))


# EN/RU: Write protocol/lifetime and porting pages in both languages, sharing only factual source links.
# Записать страницы протокола/времени жизни и переноса на обоих языках с общими фактическими source links.
def topics(root):
    for package in ("styx", "libunibus"):
        for language in (0, 1):
            suffix = ".ru.md" if language else ".md"
            title = "Протокол и время жизни" if language else "Protocol and lifetime"
            content = ("Styx uses standard 9P2000 messages and explicit fids/Qids. Attach selects the exported root; walk resolves components; open establishes access; read/write operate on that held fid; clunk releases it. Directory contents are complete stat/Dir records, including attributes, for the selected directory. A tree is never a protocol payload. A separate opt-in traversal wrapper is not ordinary raw9P."
                if package == "styx" else "Unibus uses the existing canonical S-record profile. The outer service owner handles tag0; independent client envelopes contain their own nested records. The same inner IDs can be reused by different clients. The ordinal prefix is a CRC5-protected width byte plus1..8 unsigned bytes. A sequence counter belongs to a tag/direction and renews only through its acknowledged boundary, not a new generation field in every packet.")
            if language:
                content = ("Styx использует стандартные 9P2000 сообщения и явные fid/Qid. Attach выбирает корень; walk разрешает компоненты; open устанавливает доступ; read/write работают с удержанным fid; clunk освобождает его. Каталог возвращает целые stat/Dir с атрибутами только выбранного уровня. Дерево целиком по протоколу не пересылается. Отдельный opt-in traversal wrapper не является обычным raw9P."
                    if package == "styx" else "Unibus использует существующий canonical S-record profile. Внешним tag0 владеет служебный обработчик; клиентские конверты содержат собственные вложенные записи. Клиенты могут повторять одинаковые внутренние ID. Prefix номера — width byte с CRC5 и1..8 unsigned байтов. Счётчик принадлежит тегу/направлению и сбрасывается только по подтверждённой границе, без поколения в каждом пакете.")
            write(root / package / ("docs/protocol" + suffix), f"""# {title}

Copyright (c) 2026 bhgv

[Guide](../README{suffix}) · [API](index{suffix}) · [Examples](../examples/README{suffix})

{content}

## {'Владение' if language else 'Ownership'}

{'Буфер codec принадлежит вызывающему. Decoded view не живёт дольше буфера. Успешный open/fid и native context имеют явные ссылки; снятие имени не переоткрывает ресурс. Cleanup/free допустимы только после завершения заимствующих calls. Native structs/pointers не сериализуются напрямую.' if language else 'Codec storage belongs to the caller. A decoded view cannot outlive its buffer. Successful opens/fids and native contexts have explicit references; removing a name does not reopen the resource. Cleanup/free require borrowing calls to have retired. Native structs/pointers are never serialized directly.'}

## {'Ошибки и частичный I/O' if language else 'Errors and partial I/O'}

{'Различать malformed input, временное отсутствие, EOF, подтверждённый prefix и uncertain effects. Не повторять неопределённый write автоматически. CRC не является authentication. Размеры проверяются до выдачи данных приложению; codec сам по себе не даёт прав. Исчерпание локального буфера/allocator — явная ошибка, не ложный EOF.' if language else 'Distinguish malformed input, temporary unavailability, EOF, confirmed prefixes and uncertain effects. Never automatically replay an uncertain write. CRC is not authentication. Bounds are checked before delivery to the application; a codec grants no authority. Local buffer/allocator exhaustion is an explicit failure, not fabricated EOF.'}

## {'Потоки и scope' if language else 'Threads and scope'}

{'Отдельный обслуживающий владелец принимает записи; очередь потребителя не является всем физическим каналом. Потоки библиотека не останавливает ради исследования. Для каждого изменяемого caller-owned codec context нужна собственная сериализация; независимые contexts могут исполняться параллельно. Нативные очереди имеют свои mutex/condition. Пример Styx backend синхронный, это прямо ограничивает его dispatcher.' if language else 'A separate service owner receives records; a consumer queue is not the whole physical channel. The library does not pause execution for inspection. Each mutable caller-owned codec context needs its own serialization; independent contexts can execute concurrently. Native queues provide their mutex/condition. The Styx example backend is synchronous and consequently delays its example dispatcher.'}

## {'Основной help' if language else 'Main-project help'}

- [Original native libraries](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md).
- [Ordered Unibus transport](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md).
- [Inferno Styxservers reference](https://powerman.name/Inferno/man/2/styxservers.html).
""")
            write(root / package / ("docs/porting" + suffix), f"""# {'Перенос и встраивание' if language else 'Porting and embedding'}

Copyright (c) 2026 bhgv

[Guide](../README{suffix}) · [API](index{suffix})

| Layer | Requirements | {'Граница' if language else 'Boundary'} |
| --- | --- | --- |
| Styx::codec | C99, exact uint32/uint64,8-bit bytes, byte/string primitives | Original 9P codec; no allocator/OS/threads |
| Unibus::wire | C89,8-bit bytes, caller buffers, portable32-bit limbs | Existing framing/state machines; no allocator/OS/threads |
| Hosted Styx | Linux64 libc/libm/pthread | amd64 acceptance; arm64/riscv64 source selection needs target testing |
| Unibus::queues | C99, malloc/free, pthread | Native waiting/reordering, not an interpreter scheduler |

{'Для MCU сначала собрать минимальный codec target. Приложение предоставляет физические read/write/wait/poll, память, session/fid ownership и callback policy. Не менять endian/ширину wire-полей под native типы. DMA/ISR callbacks не должны вызывать блокирующие hosted functions; передавать ownership обычному service worker. SPI/I2C/CAN/радио могут требовать отдельного packet/reliability adapter перед ordered-stream протоколом.' if language else 'For an MCU, first build the minimal codec target. The application provides physical read/write/wait/poll, memory, session/fid ownership and callback policy. Never change wire endian/field widths to match native types. DMA/ISR callbacks must not call blocking hosted functions; transfer ownership to a normal service worker. SPI/I2C/CAN/radio can need a packet/reliability adapter before an ordered-stream protocol.'}

{'Полная hosted lib9 не объявляется уже проверенным freestanding/32-bit портом. Скопированная библиотека не требует VM, GUI или соседнего репозитория. Просмотрщик/справка необязательны при линковке. Установленный CMake SDK имеет namespace target и собственные include paths.' if language else 'Complete hosted lib9 is not claimed as an already tested freestanding/32-bit port. The copied library requires no VM, GUI or sibling repository. Viewer/reference tools are optional for linking. The installed CMake SDK owns namespaced targets and include paths.'}

## {'Исходники и лицензии' if language else 'Source and licenses'}

{'SOURCES.jsonl фиксирует исходный путь и hash подготовленного алгоритма до оформления. Проверка экспорта сравнивает исполнимые токены после раскрытия readable_parts; комментарии/отступы и добавленные braces не считаются изменением алгоритма. Это не заменяет поведенческие проверки. Исходные лицензии Inferno сохранены отдельно от MIT дополнений bhgv.' if language else 'SOURCES.jsonl records the source path and prepared-algorithm hash before layout. Export checking compares executable tokens after readable_parts expansion; comments/layout and inserted braces are not algorithm edits. This does not replace behavioral tests. Original Inferno licenses remain separate from bhgv MIT additions.'}
""")


# EN/RU: Write all maintained guides after API generation; no package build invokes this development tool.
# Записать руководства после генерации API; сборка пакетов этот development tool не вызывает.
def main():
    root = Path(__file__).resolve().parents[1]
    examples(root)
    topics(root)


if __name__ == "__main__":
    main()
