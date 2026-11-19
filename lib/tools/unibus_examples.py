# Copyright (c) 2026 bhgv
# EN: Authored bilingual walkthroughs for the three libunibus teaching applications.
# RU: Авторские раздельные EN/RU руководства трёх учебных приложений libunibus.
from pathlib import Path
from .introductions import page


# EN: Write Markdown guides under base, the independent libunibus directory; no engine files are read.
# Returns None; write errors propagate. Run serially as a development tool, not during package builds.
# RU: Записать Markdown под base — независимым libunibus; файлы движка не читаются.
# Возврата нет; ошибки передаются. Последовательный development tool, не часть сборки пакета.
def main(base):
    for ru in (False, True):
        suffix = '.ru.md' if ru else '.md'
        links = []
        for kind, executable, source in [('nested','unibus-nested','nested.c'),('queues','unibus-queues','queues.c'),('bridge','unibus-9p-bridge','bridge/main.c')]:
            parent = '../' if kind == 'bridge' else ''
            relative = '../../' if kind == 'bridge' else '../'
            descriptions = {
                'nested': ('Encode tag-zero schema assignment, then feed a nested client message one byte at a time. Outer tag 7 and inner tag 1 have different scopes. This is the smallest allocation-free example and needs no physical connection.',
                           'Назначить схему через тег 0, затем подать вложенную запись побайтно. Внешний тег 7 и внутренний тег 1 имеют разные области видимости. Минимальный пример без allocation и физического соединения.'),
                'queues': ('A pthread consumer waits only for client 7 while the dispatcher delivers client 8 immediately. The barrier establishes concurrent lifetime without sleeps. Main then delivers client 7, joins the consumer and closes/frees both engines.',
                           'Pthread-потребитель ждёт только клиента 7, пока диспетчер выдаёт клиенту 8 готовую запись. Barrier задаёт совместное время жизни без sleep. Затем main выдаёт клиенту 7, делает join и закрывает/освобождает оба engine.'),
                'bridge': ('Two independent clients share S-record stdin/stdout and each owns a separate loopback 9P backend connection. RX dispatch never waits for backend replies. Worker replies are serialized as complete outer records. This optional POSIX application does not link libstyx.',
                           'Два клиента используют общий stdin/stdout S-записей и отдельные loopback-соединения с 9P backend. RX-dispatch не ждёт ответа backend. Worker-ответы выдаются целыми внешними записями под output-lock. Необязательное POSIX-приложение не линкует libstyx.')}
            body = f'# {executable}\n\nCopyright (c) 2026 bhgv\n\n[English]({kind+".md" if kind != "bridge" else "README.md"}) · [Русский]({kind+".ru.md" if kind != "bridge" else "README.ru.md"}) · [Index]({parent}README{suffix})\n\n'
            body += '## ' + ('Что и зачем' if ru else 'Purpose and design') + '\n\n' + descriptions[kind][ru] + '\n\n'
            body += '## ' + ('Сборка и запуск' if ru else 'Build and run') + f'\n\n```sh\ncmake -S . -B build -DUNIBUS_QUEUES={"OFF" if kind=="nested" else "ON"} -DBUILD_TESTING=ON\ncmake --build build -j\n'
            body += (f'./build/examples/{executable}\nctest --test-dir build -R ^{executable}$ --output-on-failure\n' if kind!='bridge' else 'python3 examples/bridge/check.py build/examples/unibus-9p-bridge\nctest --test-dir build -R ^unibus-gateway$ --output-on-failure\n') + '```\n\n'
            body += '## ' + ('Параметры и результаты' if ru else 'Parameters and results') + '\n\n'
            body += ('Аргументов командной строки нет. Код 0 означает, что все проверки примера пройдены; код 1 означает отказ; stderr содержит название этапа. stdout — диагностический PASS, а не произвольный двоичный transport.\n\n' if ru else 'No command-line arguments. Exit 0 means all example checks passed; exit 1 reports failure and stderr identifies the stage. Stdout is a PASS diagnostic, not an arbitrary binary transport.\n\n') if kind!='bridge' else (
                '| Argument | Values | Meaning |\n| --- | --- | --- |\n| argv1 | 1..65535 | Client7 loopback TCP backend port |\n| argv2 | 1..65535 | Client8 loopback TCP backend port |\n| stdin/stdout | binary S-record stream | Shared RX / serialized replies |\n\n' +
                ('Запускайте gateway через binary pipes. check.py создаёт backend-серверы и показывает корректный обмен; не посылайте текст в stdin. EOF с частичной записью даёт код 2.\n\n' if ru else 'Launch through binary pipes. check.py creates the backends and drives a valid exchange; do not type text on stdin. Partial-record EOF exits with status 2.\n\n'))
            if kind=='nested':
                body += ('Буферы 128/256 байт — учебный выбор, не предел протокола. slots содержит два назначения, реально используется одно. Проверяются tag, schema и четыре байта 21.5; MORE между фрагментами допустим. После выдачи borrowed payload нельзя перезаписывать его storage до обработки.\n\n' if ru else '128/256-byte buffers are sample capacities, not protocol limits. Two binding slots are provided; one is used. Checks cover tag, schema and the four bytes 21.5; MORE between fragments is expected. Do not overwrite storage while its borrowed payload is being consumed.\n\n')
            elif kind=='queues':
                body += ('Параметры styxordercreate: максимум сообщения 256, окно 16; wait клиента 7 ограничен 5000 мс. Barrier подтверждает запуск worker, но не точный момент входа в wait. При любой ошибке до создания worker освобождаются уже созданные engines и barrier. После создания receiver закрывается перед join; готовые записи доступны для дочитывания. Проверка unibus-example-failures внедряет четыре отказа частичной инициализации. Неожиданная ошибка join завершает учебный процесс через _Exit: без подтверждённого завершения worker нельзя освобождать заимствованное им состояние. Встроенное приложение должно выбрать свою политику такого фатального отказа. Это параметры примера, не политика всех клиентов. Старое C-имя styxorder сохраняется для совместимости исходников, зависимости от libstyx нет.\n\n' if ru else 'styxordercreate uses a maximum message of 256 and window 16; client7 wait is bounded to 5000 ms. The barrier confirms worker startup, not exact wait admission. Setup failures release every constructed engine/barrier. After worker creation, receiver close precedes join; ready records remain drainable. unibus-example-failures injects four partial-initialization failures. Unexpected join failure terminates this teaching process through _Exit: state borrowed by an unretired worker cannot be reclaimed safely. An embedding application must choose its own fatal-failure policy. These are example settings, not policy for every client. Historical styxorder C names preserve source compatibility without a libstyx dependency.\n\n')
            else:
                body += ('Внешние теги 7/8 назначаются схемой example.9p.v1 версии 1; внутренний тег 1 несёт raw 9P, тег 2 — ошибку backend. Frame максимум 8192 байта. У каждого клиента только один backend RPC in-flight: concurrent Tflush того же клиента не реализован этим учебным шлюзом. Разрыв клиента 7 не отменяет запросы клиента 8. Никакого автоматического replay записи. Сетевые timeout здесь 3 с на I/O.\n\n' if ru else 'Outer tags 7/8 bind example.9p.v1 version 1; inner tag 1 carries raw 9P and inner tag 2 reports backend failure. Frame limit is 8192 bytes. Each client has one backend RPC in flight: same-client concurrent Tflush is outside this teaching gateway. Client7 failure does not cancel client8 requests. Writes are never automatically replayed. Socket timeout is 3 seconds per I/O.\n\n')
            body += '## ' + ('Исходник и справка' if ru else 'Source and reference') + f'\n\n- [Implementation]({parent}{source})\n- [API]({relative}docs/index{suffix})\n- [Protocol]({relative}docs/protocol{suffix})\n- [Tutorial]({relative}docs/tutorial{suffix})\n- [Main project](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)\n'
            body = body.replace('styxordercreate', f'[styxordercreate]({relative}docs/reference/styxordercreate{suffix})')
            output = ('examples/bridge/README' if kind=='bridge' else 'examples/'+kind) + suffix
            page(base,output,body)
            links.append(f'- [{executable}]({"bridge/README" if kind=="bridge" else kind}{suffix})')
        page(base,'examples/README'+suffix,'# libunibus: '+('примеры' if ru else 'examples')+'\n\nCopyright (c) 2026 bhgv\n\n'+ '\n'.join(links) +f'\n\n[Introduction](../docs/introduction{suffix}) · [Tutorial](../docs/tutorial{suffix}) · [API](../docs/index{suffix})\n\n'+('Примеры файловых деревьев находятся в отдельной libstyx; gateway соединяет wire-форматы без библиотечной зависимости. Межбиблиотечная проверка в общем репозитории запускает все пять серверных моделей через gateway.' if ru else 'File-tree examples belong to separate libstyx; this gateway connects wire formats without a library dependency. Repository integration checks run all five server models through the gateway.'))


if __name__ == '__main__':
    main(Path(__file__).resolve().parents[1] / 'libunibus')
