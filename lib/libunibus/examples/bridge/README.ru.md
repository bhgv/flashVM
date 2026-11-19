# unibus-9p-bridge

Copyright (c) 2026 bhgv

[English](README.md) · [Русский](README.ru.md) · [Examples](../README.ru.md) · [Library](../../README.ru.md)

## Что и зачем

Два клиента используют общий S-record stdin/stdout, но разные backend-соединения и очереди. У них могут совпадать inner ID и 9P tags/fids. Пример связывает непрозрачный 9P с вложенными S-записями, не линкуя libstyx, DisVM или FlashVM. TCP здесь — backend учебного POSIX-приложения, не ограничение носителей библиотеки.

## Синтаксис и параметры

```text
unibus-9p-bridge PORT7 [PORT8]
```

| Параметр | Значения | Смысл |
| --- | --- | --- |
| PORT7 | Только цифры, 1..65535 | Loopback backend клиента 7 |
| PORT8 | Необязательный, тот же диапазон | Backend клиента 8; по умолчанию PORT7 |
| stdin | Binary S-record stream | Единственный transport RX owner |
| stdout | Binary S-record stream | Только целые проверяемые служебные/клиентские записи |
| stderr | Текст | Usage, ошибки setup/admission/cleanup |

Пробелы, знаки, лишние аргументы, ноль и переполнение отклоняются. Одинаковые порты создают всё равно два разных соединения. Только библиотечные/codecs paths поддерживают другие среды без POSIX; этот executable использует pthread, poll и sockets.

## Коды завершения

0 — полный EOF носителя и успешный cleanup; это не утверждение, что все клиентские RPC были успешны: inner 2 и обычный Rerror имеют собственный смысл. 1 — неверный CLI или ошибка настройки SIGPIPE. 2 — startup, carrier/protocol или cleanup failure; этот код не доказывает отсутствие уже выполненных backend effects.

## Сборка и воспроизводимый запуск

```sh
cmake -S . -B build -DUNIBUS_QUEUES=ON -DBUILD_TESTING=ON
cmake --build build -j
python3 examples/bridge/check.py build/examples/unibus-9p-bridge
python3 tests/gateway_edges.py build/examples/unibus-9p-bridge
ctest --test-dir build -R 'unibus-(gateway|bridge-)' --output-on-failure
```

Запускайте из корня пакета. Python проверка создаёт реальные loopback listeners и binary pipes. Первый тест проверяет совместимый вариант с одним портом; второй — разные порты, pending RPC, ошибки и shutdown. Для своего подключения передавайте stdout/stdin через binary pipes; не печатайте текст в stdin.

## Порядок работы

1. [bridge_run](lifecycle.c#L136) создаёт private wake pipe и два соединения. [bridge_worker_start](lifecycle.c#L20) поглощает каждый fd и откатывает partial init при ошибке.
2. Служба объявляет внешние теги 7/8 через тег 0, схема example.9p.v1 версии 1. Peer присылает своё назначение до клиентских записей; точные повторы разрешены, конфликт запрещён immutable bindings API.
3. [bridge_receive](receive.c#L61) ждёт stdin либо сообщение о потере вывода. [dispatch](receive.c#L25) обрабатывает служебный тег 0 внутри RX; клиентам он недоступен.
4. [bridge_enqueue](worker.c#L18) копирует 9P frame либо ставит маркер ошибки в FIFO клиента. RX не вызывает [bridge_reply](io.c#L94) и не ждёт backend execution.
5. [bridge_worker](worker.c#L89) выполняет RPC своего клиента по одному и выдаёт inner tag 1 (ответ) либо 2 (ошибка) внутри его outer tag. Ошибки также идут в FIFO-порядке.
6. [bridge_worker_stop](lifecycle.c#L57) прекращает admission; [bridge_worker_finish](lifecycle.c#L78) делает join, затем освобождает socket, остаток FIFO и синхронизацию. Номер fd не закрывается и не переиспользуется до join.

## Область отказа и последствия

| Событие | Реакция | Последствия для операций |
| --- | --- | --- |
| Неверный inner frame известного клиента | Inner 2 через FIFO только этого клиента | В backend не передавался; следующие правильные запросы допустимы |
| Backend принял запрос, затем отключился/дал неверную длину/усечённый ответ | Sticky failure только этого клиента; inner 2 для последующих запросов | Эффект первого запроса неизвестен; replay/reconnect нет. Другой клиент работает |
| Повреждённый outer frame или неизвестный transport tag/schema | Завершение с кодом 2 | Уже принятые запросы дочитываются; разрыв не доказывает отсутствие эффектов |
| Нехватка памяти для queue copy | Диагностика admission и код 2 | Не терять молча/не вводить произвольную квоту; ранее принятые запросы остаются принятыми |
| EOF между записями | Drain принятых FIFO, join, код 0 | Ответы могут прийти после half-close stdin |
| EOF внутри записи | Код 2 | Не выдавать частичную запись и не пытаться угадать resynchronization |
| Потерян stdout | Sticky terminal output; wake RX; прекращение очередной работы, join, код 2 | Частичные/выполненные эффекты не отменяются; остаток очереди может остаться неисполненным |
| Ошибка native join | Фатальный _Exit(2) | Не освобождать память, которой может пользоваться незавершившийся worker |

## Частичный I/O и границы профиля

[bridge_exact](io.c#L63) продолжает только оставшийся диапазон после короткой операции и EINTR. Нулевая запись означает отсутствие прогресса. После частичной ошибки [bridge_record](io.c#L72) запрещает любые дальнейшие записи в это физическое направление до завершения процесса: следующий frame не восстановит потерянную границу.

Socket timeout — 3 с на syscall, не абсолютный deadline RPC. Медленный backend может продлевать RPC частичным прогрессом; другой worker остаётся независимым. Физическое обратное давление stdout может блокировать вывод workers; это не ожидание клиентской очереди и не разрешение терять записи. Private pipe делает nonblocking только свой writer, не меняет флаги унаследованных stdin/stdout; если stdout уже nonblocking, EAGAIN ожидается через poll, а backend EAGAIN остаётся socket timeout и не добавляет новый Unibus-тег.

Максимум 9P frame — 8192 байта. Шлюз проверяет длину, но не разбирает 9P type/tag и содержимое: это работа 9P-клиента. Обычный Rerror передаётся неизменённым в inner 1, не превращается в разрыв connection и не мешает следующему запросу. Пример не реализует concurrent Tflush того же клиента, спонтанные backend-уведомления, автонаращивание физических линий, авторизацию, TLS или ordered-prefix профиль. Для этих возможностей нужны соответствующие service/PHY механизмы основного проекта; готовый codec не включает их автоматически.

[Private contracts](bridge.h) · [Wire API](../../docs/index.ru.md) · [Protocol](../../docs/protocol.ru.md) · [SDK](../../docs/embedding.ru.md) · [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
