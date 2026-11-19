# Copyright (c) 2026 bhgv
# EN: Author-maintained gateway command reference, request flow and failure/lifetime tables.
# RU: Авторская справка команды шлюза, потока запросов и таблиц отказов/времени жизни.
from pathlib import Path
import re
from .introductions import page


# EN: Find a private function's physical line in borrowed base/filename. Return relative Markdown source link.
# Function must exist or generation fails; no approximate anchors are silently emitted.
# RU: Найти физическую строку private function в base/filename. Вернуть относительную Markdown-ссылку.
# При отсутствии функции остановить генерацию; приблизительные anchors не выдаются.
def function(base, filename, name):
    source = (base / filename).read_text()
    match = re.search(r'^.*\b' + re.escape(name) + r'\([^;\n]*\)\n\{', source, re.M)
    if match is None:
        raise ValueError(f'{filename}: missing definition {name}')
    line = source.count('\n', 0, match.start()) + 1
    return f'[{name}]({filename}#L{line})'


# EN: Write independent EN/RU command help under base (the libunibus package), using real source anchors.
# Only documentation is changed; private helper names are not promoted into a public library ABI.
# RU: Записать EN/RU command help под base (пакет libunibus), с настоящими source anchors.
# Менять только документацию; private helper names не становятся публичным ABI библиотеки.
def main(base):
    source = base / 'examples/bridge'
    funcs = {name: function(source, filename, name) for filename, names in [
        ('lifecycle.c', ['bridge_run', 'bridge_worker_start', 'bridge_worker_stop', 'bridge_worker_finish']),
        ('receive.c', ['bridge_receive', 'dispatch']), ('worker.c', ['bridge_enqueue', 'bridge_worker']),
        ('io.c', ['bridge_exact', 'bridge_record', 'bridge_reply'])] for name in names}
    for ru in (False, True):
        suffix = '.ru.md' if ru else '.md'
        text = '# unibus-9p-bridge\n\nCopyright (c) 2026 bhgv\n\n'
        text += f'[English](README.md) · [Русский](README.ru.md) · [Examples](../README{suffix}) · [Library](../../README{suffix})\n\n'
        text += '## ' + ('Что и зачем' if ru else 'Purpose') + '\n\n'
        text += ('Два клиента используют общий S-record stdin/stdout, но разные backend-соединения и очереди. У них могут совпадать inner ID и 9P tags/fids. Пример связывает непрозрачный 9P с вложенными S-записями, не линкуя libstyx, DisVM или FlashVM. TCP здесь — backend учебного POSIX-приложения, не ограничение носителей библиотеки.\n\n' if ru else 'Two clients share S-record stdin/stdout but own different backend connections and queues. They may reuse inner IDs and 9P tags/fids. This example carries opaque 9P inside nested records without linking libstyx, DisVM or FlashVM. TCP is this POSIX demonstration backend, not a library carrier restriction.\n\n')
        text += '## ' + ('Синтаксис и параметры' if ru else 'Syntax and parameters') + '\n\n```text\nunibus-9p-bridge PORT7 [PORT8]\n```\n\n'
        text += ('| Параметр | Значения | Смысл |\n| --- | --- | --- |\n| PORT7 | Только цифры, 1..65535 | Loopback backend клиента 7 |\n| PORT8 | Необязательный, тот же диапазон | Backend клиента 8; по умолчанию PORT7 |\n| stdin | Binary S-record stream | Единственный transport RX owner |\n| stdout | Binary S-record stream | Только целые проверяемые служебные/клиентские записи |\n| stderr | Текст | Usage, ошибки setup/admission/cleanup |\n' if ru else '| Parameter | Values | Meaning |\n| --- | --- | --- |\n| PORT7 | Digits only, 1..65535 | Client 7 loopback backend |\n| PORT8 | Optional, same range | Client 8 backend; defaults to PORT7 |\n| stdin | Binary S-record stream | Sole transport RX owner |\n| stdout | Binary S-record stream | Complete checked service/client records only |\n| stderr | Text | Usage and setup/admission/cleanup diagnostics |\n')
        text += '\n' + ('Пробелы, знаки, лишние аргументы, ноль и переполнение отклоняются. Одинаковые порты создают всё равно два разных соединения. Только библиотечные/codecs paths поддерживают другие среды без POSIX; этот executable использует pthread, poll и sockets.\n\n' if ru else 'Whitespace, signs, extra arguments, zero and overflow are rejected. Equal ports still create two different connections. This executable uses pthread, poll and sockets; other environments use the library/codec integration paths.\n\n')
        text += '## ' + ('Коды завершения' if ru else 'Exit status') + '\n\n'
        text += ('0 — полный EOF носителя и успешный cleanup; это не утверждение, что все клиентские RPC были успешны: inner 2 и обычный Rerror имеют собственный смысл. 1 — неверный CLI или ошибка настройки SIGPIPE. 2 — startup, carrier/protocol или cleanup failure; этот код не доказывает отсутствие уже выполненных backend effects.\n\n' if ru else '0 means complete carrier EOF and successful cleanup, not success of every client RPC: inner 2 and ordinary Rerror carry their own outcomes. 1 means invalid CLI or SIGPIPE setup failure. 2 means startup, carrier/protocol or cleanup failure; it does not prove that no backend effect occurred.\n\n')
        text += '## ' + ('Сборка и воспроизводимый запуск' if ru else 'Build and reproducible run') + '''

```sh
cmake -S . -B build -DUNIBUS_QUEUES=ON -DBUILD_TESTING=ON
cmake --build build -j
python3 examples/bridge/check.py build/examples/unibus-9p-bridge
python3 tests/gateway_edges.py build/examples/unibus-9p-bridge
ctest --test-dir build -R 'unibus-(gateway|bridge-)' --output-on-failure
```

'''
        text += ('Запускайте из корня пакета. Python проверка создаёт реальные loopback listeners и binary pipes. Первый тест проверяет совместимый вариант с одним портом; второй — разные порты, pending RPC, ошибки и shutdown. Для своего подключения передавайте stdout/stdin через binary pipes; не печатайте текст в stdin.\n\n' if ru else 'Run from the package root. The Python checks create real loopback listeners and binary pipes. The first checks the compatible one-port form; the second covers distinct ports, pending RPCs, errors and shutdown. Integrate through binary pipes; do not type text on stdin.\n\n')
        text += '## ' + ('Порядок работы' if ru else 'Execution flow') + '\n\n'
        if ru:
            text += f'''1. {funcs['bridge_run']} создаёт private wake pipe и два соединения. {funcs['bridge_worker_start']} поглощает каждый fd и откатывает partial init при ошибке.
2. Служба объявляет внешние теги 7/8 через тег 0, схема example.9p.v1 версии 1. Peer присылает своё назначение до клиентских записей; точные повторы разрешены, конфликт запрещён immutable bindings API.
3. {funcs['bridge_receive']} ждёт stdin либо сообщение о потере вывода. {funcs['dispatch']} обрабатывает служебный тег 0 внутри RX; клиентам он недоступен.
4. {funcs['bridge_enqueue']} копирует 9P frame либо ставит маркер ошибки в FIFO клиента. RX не вызывает {funcs['bridge_reply']} и не ждёт backend execution.
5. {funcs['bridge_worker']} выполняет RPC своего клиента по одному и выдаёт inner tag 1 (ответ) либо 2 (ошибка) внутри его outer tag. Ошибки также идут в FIFO-порядке.
6. {funcs['bridge_worker_stop']} прекращает admission; {funcs['bridge_worker_finish']} делает join, затем освобождает socket, остаток FIFO и синхронизацию. Номер fd не закрывается и не переиспользуется до join.

'''
        else:
            text += f'''1. {funcs['bridge_run']} creates a private wake pipe and two connections. {funcs['bridge_worker_start']} consumes each fd and unwinds partial initialization failures.
2. The service declares outer tags 7/8 under tag 0, schema example.9p.v1 version 1. The peer sends its assignments before client traffic; identical repeats are allowed, conflicts rejected by immutable bindings.
3. {funcs['bridge_receive']} waits for stdin or an output-loss wake. {funcs['dispatch']} handles service tag 0 inside RX; clients never receive it.
4. {funcs['bridge_enqueue']} copies the 9P frame or an error marker into that client's FIFO. RX never calls {funcs['bridge_reply']} or waits for backend execution.
5. {funcs['bridge_worker']} executes one RPC at a time for its own client and emits inner tag 1 (reply) or 2 (error) inside that outer client tag. Errors preserve FIFO order too.
6. {funcs['bridge_worker_stop']} ends admission; {funcs['bridge_worker_finish']} joins before releasing socket, remaining FIFO and synchronization. The fd number cannot be closed/reused before join.

'''
        text += '## ' + ('Область отказа и последствия' if ru else 'Failure scope and effects') + '\n\n'
        if ru:
            text += '''| Событие | Реакция | Последствия для операций |
| --- | --- | --- |
| Неверный inner frame известного клиента | Inner 2 через FIFO только этого клиента | В backend не передавался; следующие правильные запросы допустимы |
| Backend принял запрос, затем отключился/дал неверную длину/усечённый ответ | Sticky failure только этого клиента; inner 2 для последующих запросов | Эффект первого запроса неизвестен; replay/reconnect нет. Другой клиент работает |
| Повреждённый outer frame или неизвестный transport tag/schema | Завершение с кодом 2 | Уже принятые запросы дочитываются; разрыв не доказывает отсутствие эффектов |
| Нехватка памяти для queue copy | Диагностика admission и код 2 | Не терять молча/не вводить произвольную квоту; ранее принятые запросы остаются принятыми |
| EOF между записями | Drain принятых FIFO, join, код 0 | Ответы могут прийти после half-close stdin |
| EOF внутри записи | Код 2 | Не выдавать частичную запись и не пытаться угадать resynchronization |
| Потерян stdout | Sticky terminal output; wake RX; прекращение очередной работы, join, код 2 | Частичные/выполненные эффекты не отменяются; остаток очереди может остаться неисполненным |
| Ошибка native join | Фатальный _Exit(2) | Не освобождать память, которой может пользоваться незавершившийся worker |

'''
        else:
            text += '''| Event | Response | Operation effects |
| --- | --- | --- |
| Invalid inner frame on a known client | Inner 2 through only that client's FIFO | Never forwarded; later valid requests remain usable |
| Backend consumed request then disconnected/truncated reply/reported an invalid length | Sticky failure on that client; inner 2 for later requests | First effect is uncertain; no replay/reconnect. Other client progresses |
| Invalid outer frame or unknown transport tag/schema | Exit 2 | Previously admitted requests drain; disconnect does not prove absent effects |
| Queue-copy allocation failure | Admission diagnostic and exit 2 | No silent drop/arbitrary quota; earlier admitted requests remain admitted |
| EOF between records | Drain accepted FIFOs, join, exit 0 | Replies may arrive after stdin half-close |
| EOF inside a record | Exit 2 | Never publish a partial record or guess resynchronization |
| Lost stdout | Sticky terminal output; wake RX; stop further queued work, join, exit 2 | Partial/completed effects are not undone; queued work may remain unexecuted |
| Native join failure | Fatal _Exit(2) | Never reclaim memory an unretired worker might still use |

'''
        text += '## ' + ('Частичный I/O и границы профиля' if ru else 'Partial I/O and profile boundaries') + '\n\n'
        if ru:
            text += f'''{funcs['bridge_exact']} продолжает только оставшийся диапазон после короткой операции и EINTR. Нулевая запись означает отсутствие прогресса. После частичной ошибки {funcs['bridge_record']} запрещает любые дальнейшие записи в это физическое направление до завершения процесса: следующий frame не восстановит потерянную границу.

Socket timeout — 3 с на syscall, не абсолютный deadline RPC. Медленный backend может продлевать RPC частичным прогрессом; другой worker остаётся независимым. Физическое обратное давление stdout может блокировать вывод workers; это не ожидание клиентской очереди и не разрешение терять записи. Private pipe делает nonblocking только свой writer, не меняет флаги унаследованных stdin/stdout; если stdout уже nonblocking, EAGAIN ожидается через poll, а backend EAGAIN остаётся socket timeout и не добавляет новый Unibus-тег.

Максимум 9P frame — 8192 байта. Шлюз проверяет длину, но не разбирает 9P type/tag и содержимое: это работа 9P-клиента. Обычный Rerror передаётся неизменённым в inner 1, не превращается в разрыв connection и не мешает следующему запросу. Пример не реализует concurrent Tflush того же клиента, спонтанные backend-уведомления, автонаращивание физических линий, авторизацию, TLS или ordered-prefix профиль. Для этих возможностей нужны соответствующие service/PHY механизмы основного проекта; готовый codec не включает их автоматически.

'''
        else:
            text += f'''{funcs['bridge_exact']} advances only the remaining span after a short operation/EINTR. Zero write means no progress. After a partial error, {funcs['bridge_record']} forbids every further write to that physical direction until process exit: another frame cannot restore its lost boundary.

Socket timeout is 3 seconds per syscall, not an absolute RPC deadline. A slow backend can extend an RPC through partial progress; the other worker remains independent. Physical stdout backpressure can block output workers; this is distinct from waiting on a client's queue and does not permit losing records. The private wake pipe changes only its own descriptor flags, never inherited stdin/stdout flags; already-nonblocking stdout waits through poll on EAGAIN, while backend EAGAIN remains a socket timeout, and adds no Unibus tag.

Maximum 9P frame is 8192 bytes. The gateway validates length, not 9P type/tag or application fields; the 9P client validates those. An ordinary Rerror is forwarded unchanged under inner 1, does not break the connection and does not prevent a later request. This example does not implement same-client concurrent Tflush, spontaneous backend notifications, automatic physical-lane scaling, authorization, TLS or the ordered-prefix profile. These need the corresponding project service/PHY mechanisms; a codec does not automatically enable them.

'''
        text += f'[Private contracts](bridge.h) · [Wire API](../../docs/index{suffix}) · [Protocol](../../docs/protocol{suffix}) · [SDK](../../docs/embedding{suffix}) · [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)\n'
        page(base, 'examples/bridge/README' + suffix, text)


if __name__ == '__main__':
    main(Path(__file__).resolve().parents[1] / 'libunibus')
