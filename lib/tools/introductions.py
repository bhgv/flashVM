# Copyright (c) 2026 bhgv
# EN: Maintain conceptual introductions and step-by-step server tutorials in two languages.
# RU: Хранить концептуальное введение и пошаговое руководство сервера на двух языках.
from pathlib import Path


# EN: Write a UTF-8 page; base is the owned package root, name a relative documentation path,
# and body authored Markdown. Creates parents; filesystem failures propagate to the caller.
# RU: Записать UTF-8 страницу; base — корень пакета, name — относительный путь,
# body — авторский Markdown. Создать родителей; ошибки ФС передаются вызывающему.
def page(base, name, body):
    path = base / name
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(body.strip() + '\n')


# EN: Emit authored introductions and tutorials into root's two independent packages.
# root is lib/, not a build dependency. Returns None; write errors propagate; run serially.
# RU: Записать авторские введения/руководства в два независимых пакета root (lib/).
# Это не зависимость сборки. Возврата нет; ошибки записи передаются; запуск последовательный.
def main(root):
    for package in ('styx', 'libunibus'):
        base = root / package
        styx = package == 'styx'
        title = 'libstyx' if styx else 'libunibus'
        for ru in (False, True):
            suffix = '.ru.md' if ru else '.md'
            intro = ('''Ресурс как имя

Файл в этом интерфейсе — именованный ресурс, а не обязательно байты на диске. Это может быть датчик, запись БД, команда, окно или поток данных. Клиент обходит имена и открывает ресурс; сервер решает, что стоит за именем. Ему не нужно заранее перечислять всё дерево. Права и подготовка доступной клиенту проекции остаются обязанностью сервера.

Слои библиотеки

`Styx::codec` кодирует стандартные сообщения 9P2000 в предоставленные буферы. Здесь нет сокетов, потоков, malloc и подключения к DisVM. `Styx::server` добавляет исходную hosted libstyx и её callback-интерфейс. `Styx::client` предоставляет существующий нативный клиент. `Styx::carrier` отделяет физический ввод-вывод. Полный hosted-вариант сейчас рассчитан на Linux64; перенос codec не означает готового серверного порта на MCU.

Жизненный цикл

Tversion согласует диалект и msize; Tattach создаёт корневой fid. Twalk разрешает компоненты пути относительно fid, Topen открывает результат. Tread/Twrite работают с уже открытым fid и offset; повторный Twalk перед каждой операцией не требуется. Tread каталога отдаёт последовательность Dir только его непосредственных детей. Tclunk освобождает fid; обрыв соединения требует очистки всех удержаний сессии. Имя может исчезнуть после открытия: отдельно продумайте срок жизни открытого объекта.

Выбор применения

Используйте 9P для публикации пространства имён и привычных файловых операций. Для маленького устройства можно начать с codec и собственных session/fid/transport адаптеров. Для Linux — с готового callback-сервера. Без собственного контроля доступа эти учебные серверы не следует открывать недоверенным клиентам. Синхронный учебный backend не является универсальным асинхронным сервером.'''
                     if styx else '''Независимые разговоры в одном соединении

libunibus переносит сообщения разных клиентов по общему соединению, сохраняя границы записей и пространства тегов. Это библиотека связи, а не файловая система и не интерпретатор. Она может нести 9P, команды приложения или другой согласованный контент. Чтобы публиковать файлы, приложение отдельно подключает сервер 9P; библиотека от libstyx не зависит.

Слои и владение

`Unibus::wire` — C89 codec и инкрементальные автоматы над буферами вызывающего; без malloc, ОС и потоков. `Unibus::queues` — необязательные C99/pthread очереди и упорядочивание. Код приложения владеет физическим каналом, обслуживающим потоком и временем жизни клиентов. Наличие queue API само по себе не запускает универсальный сетевой брокер.

Два уровня тегов

Транспортный тег 0 принадлежит обслуживанию соединения: согласованию назначений, схем и управляющих операций. Другие внешние теги маршрутизируют клиентов. В payload клиентской записи можно вложить свои S-записи: клиент A и клиент B могут оба использовать внутренний тег 1. Клиент получает свою вложенную запись, а не доступ к тегу 0 и общему RX. Рабочий поток ждёт свою очередь; это не должно останавливать RX других клиентов.

Когда использовать

Выберите голый wire для маленького контроллера со своим event loop; optional queues — когда нужны нативные ожидающие потребители. Не добавляйте этот слой к простому 9P-соединению без необходимости мультиплексирования. Пример gateway демонстрирует две независимые очереди и два backend-соединения, а не все функции основного брокера DisVM/FlashVM. Дополнительные физические каналы, политика нагрузки и авторизация — отдельные обязанности интеграции.''') if ru else (
                '''Resources as names

A file is a named resource, not necessarily disk storage. A sensor, database row, command endpoint, window or data stream can have a pathname. Clients walk names and open resources; the server determines what each name means. It need not pre-enumerate an entire tree. Access checks and each client's exported projection remain server responsibilities.

Library layers

`Styx::codec` converts standard 9P2000 messages in caller-provided buffers without sockets, threads, allocation or DisVM. `Styx::server` adds the original hosted libstyx callback interface. `Styx::client` provides the existing native client. `Styx::carrier` separates physical I/O. The complete hosted distribution currently targets Linux64; portable codec support does not imply a complete MCU server port.

Request lifecycle

Tversion negotiates dialect and msize; Tattach creates a root fid. Twalk resolves pathname components relative to a fid, and Topen opens the result. Tread/Twrite operate on the held fid and offset; another walk is not required before every operation. A directory Tread returns encoded Dir records for immediate children only. Tclunk releases a fid; disconnect must clean up all session-held objects. A name may disappear after opening: design held-object lifetime separately.

Choosing this interface

Use 9P to publish a namespace through familiar file operations. On a small device, start with the codec and supply session/fid/transport adapters. On Linux, start with the callback server. The teaching servers need an application access policy before exposure to untrusted peers. A synchronous sample backend is not a universal asynchronous service.'''
                if styx else '''Independent conversations over one connection

libunibus carries messages for different clients over a shared connection while preserving record boundaries and tag namespaces. It is a communication library, not a filesystem or interpreter. Payloads can carry 9P, application commands or another negotiated format. File publication needs a separate 9P server; libunibus does not depend on libstyx.

Layers and ownership

`Unibus::wire` provides C89 codecs and incremental state machines over caller-owned buffers, without allocation, OS or threads. Optional `Unibus::queues` adds C99/pthread queues and ordering. The application owns the physical carrier, service thread and client lifetimes. Queue APIs do not themselves launch a universal network broker.

Two levels of tags

Transport tag 0 belongs to connection service: assignments, schemas and control operations. Other outer tags route clients. A client payload may contain nested S-records: clients A and B may both use inner tag 1. Clients receive their own nested messages, not access to tag 0 or shared RX. A worker waits for its queue without stopping RX for other clients.

When to use it

Choose wire alone for a small controller with its own event loop; select optional queues for native waiting consumers. A simple 9P connection need not add multiplexing. The gateway demonstrates two independent queues and backend connections, not all features of the DisVM/FlashVM broker. Extra physical lanes, load policy and authorization remain separate integration responsibilities.''')
            # The authored prose uses short section labels separated by blank lines.
            blocks = intro.split('\n\n')
            body = '\n\n'.join(('## ' + b if '\n' not in b and len(b) < 90 else b) for b in blocks)
            navigation = f'[README](../README{suffix}) · [API](index{suffix}) · [Tutorial](tutorial{suffix}) · [Examples](../examples/README{suffix})'
            page(base, 'docs/introduction' + suffix, f'# {title}: ' + ('что и зачем' if ru else 'what and why') + '\n\nCopyright (c) 2026 bhgv\n\n' + navigation + '\n\n' + body + '\n\n## ' + ('Порядок изучения' if ru else 'Reading order') + '\n\n' + (
                f'Начните с [пошагового руководства](tutorial{suffix}), затем выберите [пример](../examples/README{suffix}). Точные параметры и результаты — в [справочнике функций](index{suffix}); wire и lifetime — в [протоколе](protocol{suffix}); границы переносимости — в [переносе](porting{suffix}).'
                if ru else f'Start with the [tutorial](tutorial{suffix}), choose an [example](../examples/README{suffix}), then consult the [function dictionary](index{suffix}). See [protocol/lifetime](protocol{suffix}) and [porting](porting{suffix}) for integration boundaries.'))
            steps = ('''1. Соберите hosted-вариант командами из README. Запустите `styx-flat 0`: первая строка stdout сообщает реальный порт. Откройте `status` через `styx-cat PORT status`; ожидается `ready`.
2. Прочитайте [catalog.h](../examples/catalog.h): CatalogRow описывает один объект; catalog_query получает вид запроса, ID, начальную позицию, выходной массив, ёмкость и число результатов. Корень имеет ID 0. Не используйте адрес объекта как wire ID.
3. Прочитайте [fixed.c](../examples/fixed.c). Для stat верните ровно найденный объект, для list — одну страницу непосредственных детей. Стабильность ID нужна независимо от текущего имени.
4. Прочитайте [server.c](../examples/server.c): walk_node разрешает компонент; open_node проверяет режим; read_node различает файл и каталог; attributes заполняет Dir; close_node снимает удержание. Строки Dir должны дожить до упаковки. Успех возвращается только после успешной операции backend.
5. Разделите скрытие имени и уничтожение открытого объекта. [live.c](../examples/live.c) использует catalog_pin/catalog_unpin. Проверьте открытие title, удаление имени через ctl и чтение через прежний fid, затем Tclunk.
6. Перейдите к database и HTTP примерам. Списки получаются по запросу, не фиксируются на время mount. Offset каталога — число кодированных байт, а не номер ребёнка; в ответ помещаются только целые Dir. Малый буфер и исчезновение объекта — отдельные ошибки, не EOF по умолчанию.
7. Перед внедрением задайте собственную attach-проекцию, пользователей и проверки rwx в операциях, границы сообщений и отмену долгих запросов. Учебные callback-вызовы синхронны. Для долгого I/O изучите deferred-read контракты libstyx, а не блокируйте чужих клиентов невидимым ожиданием.'''
                     if styx else '''1. Соберите и запустите `unibus-nested`. Откройте [nested.c](../examples/nested.c): все buffers принадлежат приложению; первый пример не создаёт сокет или поток.
2. Создайте назначение внешнего тега 7, schema example.sensor.v1 и версии 1. Закодируйте служебное сообщение тегом 0 и примените его до выдачи клиентских записей. Проверяйте каждый результат codec, ёмкость и consumed.
3. Вложите запись внутреннего тега 1 в payload внешнего тега 7. Передавайте получившийся буфер в sstream частями; MORE означает необходимость продолжения, не ошибку и не конец сообщения. После готового outer record проверяйте назначение и отдавайте payload нужному клиенту.
4. Запустите [queues.c](../examples/queues.c). Один потребитель ждёт свой тег, другой уже имеет сообщение. Это проверяет изоляцию ожиданий. Приложение отвечает за запуск service worker; каждый конец соединения имеет собственный RX owner.
5. Прочитайте [gateway](../examples/bridge/README.ru.md). main читает общую stdin, dispatch кладёт вложенную 9P-запись в FIFO клиента, worker обслуживает отдельный backend socket, а output-lock не даёт двум ответам смешать байты. Одинаковые inner tag и fid допустимы в разных клиентах.
6. Согласуйте ordering-профиль отдельно. Базовый gateway не оборачивает записи sequence-prefix. Для ordered-профиля используйте готовые prefix/packet/order API: uint64 номер передаётся 1–8 байтами с длиной и CRC5, не native struct. Не меняйте wire-профиль молча.
7. Определите shutdown и неопределённые эффекты. Частичный physical read/write требует продолжения, а не повторения команды. Потеря ответа не доказывает, что запись не выполнена. EOF с незавершённой S-записью отклоняется. Повторное назначение в простом bindings-примере ограничено; барьер ordered API — отдельный контракт.''') if ru else (
                '''1. Build the hosted configuration from README. Run `styx-flat 0`; stdout reports the actual port. Read `status` using `styx-cat PORT status`; expect `ready`.
2. Read [catalog.h](../examples/catalog.h). CatalogRow describes one object. catalog_query receives operation, ID, starting position, output array, capacity and result count. Root ID is 0; native pointers are not wire identities.
3. Read [fixed.c](../examples/fixed.c). Stat returns the selected object; list returns one immediate-child page. Keep identities stable independently of names.
4. Read [server.c](../examples/server.c): walk_node resolves a component, open_node validates mode, read_node distinguishes file/directory reads, attributes fills Dir, and close_node releases the pin. Dir strings must survive encoding. Report success only after successful backend work.
5. Separate hiding a name from destroying an opened object. [live.c](../examples/live.c) uses catalog_pin/catalog_unpin. Test opening title, hiding its name through ctl, reading the original fid and finally Tclunk.
6. Advance to database and HTTP examples. Queries fetch live pages rather than freezing the tree at mount. Directory offsets count encoded bytes, not children; return complete Dir records only. Small buffers and disappeared objects are errors, not automatic EOF.
7. Before deployment, supply an attach projection, identities and operation-level rwx checks, message bounds and cancellation. Teaching callbacks are synchronous. For slow I/O consult libstyx deferred-read contracts rather than silently blocking unrelated clients.'''
                if styx else '''1. Build and run `unibus-nested`. Read [nested.c](../examples/nested.c): the application owns all buffers, and this example creates no socket or thread.
2. Assign outer tag 7, schema example.sensor.v1 and version 1. Encode/apply control tag 0 before releasing client traffic. Check every codec result, capacity and consumed count.
3. Nest inner tag 1 inside outer tag 7. Feed the resulting bytes into sstream in fragments. MORE asks for continuation; it is neither a failure nor end-of-message. On a complete outer record, validate the binding and deliver its payload to that client.
4. Run [queues.c](../examples/queues.c). One consumer waits for its tag while another has a ready message. This checks independent waits. The application starts the service worker; each connection end owns its own RX loop.
5. Read the [gateway](../examples/bridge/README.md). main owns stdin RX, dispatch enqueues nested opaque 9P for a client, each worker owns its backend socket, and an output lock prevents interleaved reply bytes. Different clients may reuse inner tags and fids.
6. Negotiate ordering separately. The basic gateway has no sequence prefix. For the ordered profile use the existing prefix/packet/order API: uint64 sequence, 1–8 bytes, explicit length and CRC5, never a native struct. Do not silently change wire profiles.
7. Define shutdown and uncertain effects. Partial physical I/O requires continuation, not command replay. A lost reply does not prove a write never happened. Truncated S-record EOF is rejected. Simple bindings have limited reassignment behavior; ordered API barriers are a separate contract.''')
            page(base, 'docs/tutorial' + suffix, f'# {title}: ' + ('от первого запроса к своему серверу' if ru else 'from the first request to your service') + '\n\nCopyright (c) 2026 bhgv\n\n' + navigation.replace(f'[Tutorial](tutorial{suffix})', f'[Introduction](introduction{suffix})') + '\n\n' + steps + '\n\n## ' + ('Проверка результата' if ru else 'Acceptance') + '\n\n```sh\nctest --test-dir build --output-on-failure  # Run the built examples and regression checks.\n```\n\n' + ('Проверяйте обычный запрос, малый буфер, неверное имя, частичный ввод, разрыв и освобождение удержанных объектов. Тестовый transport не заменяет проверки драйвера выбранной физической среды.' if ru else 'Check normal requests, short buffers, invalid names, fragmented input, disconnect and release of held objects. Test transport coverage does not replace testing your physical driver.'))


if __name__ == '__main__':
    main(Path(__file__).resolve().parents[1])
