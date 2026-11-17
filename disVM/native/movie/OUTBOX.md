# Automatic native terminal output / Автоматическая native-отправка терминалу

The movie SDK now turns the runtime's scoped source notices into a **bounded,
pollable outbox and a prefix-preserving writer**. A host no longer needs its own
movie-specific notice callback, state encoder or partial-send buffer. Source
processes only publish native invalidations; formatting and transport writes run
on the connection's native I/O owner. No Limbo loop or guest suspension is used.

SDK теперь сам подписывается на источник объявления, будит native I/O owner,
кодирует актуальное состояние и сохраняет недописанный остаток записи. Смена
провайдера и завершение задания не требуют команды от терминала. Здесь остаётся
прежний формат `flashvm.movie.offer`, version1, описанный в
[контракте объявления](../../runtime/TERMINAL-ANNOUNCEMENTS.md).

## API

| API | Contract / Контракт |
| --- | --- |
| [`styxmovieofferwatch(source, tag)`](publisher.h) | Owned `StyxEvents` subscriber or NULL; native descriptor readiness and exactly one complete declaration/offer pair per successful read. / Свой subscriber, fd readiness, одна пара на read. |
| [`styxmoviewriter(source, tag)`](writer.h) | Owned serializer or NULL, including its own independent subscriber and bounded pending buffer. / Свой writer с очередью и pending buffer. |
| `styxmoviewriterstep(writer, callback, state)` | At most one native callback; returns precise `DVM_IoResult`. Never waits for a source process. / Не более одного callback, точное подтверждение. |
| `styxmoviewriterfd(writer)` | Borrowed readiness fd; -1 for ended/failed/invalid writer. Never read or close it. / Borrowed fd готовности, не читать и не закрывать. |
| `styxmoviewriterpending(writer)` | Number of unsent staged bytes. Watch transport writability while nonzero. / Число ещё не подтверждённых staged bytes. |
| `styxmoviewriterfree(writer)` | Detach subscription and consume writer after I/O users retire; source callbacks own their independent lifetime. / Освободить после I/O users, без ожидания source callbacks. |

All inputs are native capabilities/values. Create the writer with the source
already granted to this connection and a **host-reserved unused outgoing tag**.
The tag is not allocated globally and gives no authority. Each constructor call
creates an independent queue; authorized recipients do not steal each other's
announcements. The source and journal stay pinned through already admitted native
callbacks, including detach races.

Это Linux64 native SDK поверх существующей очереди `StyxEvents`, исходных lib9/
libstyx и обычной VFS. Ни сетевые адреса, ни native/managed pointers не передаются
в wire-формате. Новые 9P-сообщения, графические зависимости и изменения ABI
Limbo-модулей не добавлены. Версии runtime и FlashVM этим SDK-изменением не меняются.

## I/O owner integration / Интеграция I/O owner

```c
StyxMovieWriter *writer = styxmoviewriter(granted_source, reserved_outgoing_tag);
/* On native source readiness or transport writability, on the same I/O owner: */
DVM_IoResult output = styxmoviewriterstep(writer, write_control_prefix, transport);
```

`write_control_prefix(state, bytes, size, out)` borrows bytes only until return.
It initializes a complete precise acknowledgement: confirmed `count`, `status`,
`stop`, and `DVM_IO_PREFIX`; include `DVM_IO_UNCERTAIN` if effects beyond that
prefix are unknown. The callback runs outside source/queue locks. It may use the
host's existing nonblocking transport; no native socket or file fallback is opened
by the writer. Retire users before freeing it. One writer has one exclusive I/O
caller; notification producers never call this callback.

The host's existing event loop handles the result as follows:

| Result/state | Next action / Следующее действие |
| --- | --- |
| `AGAIN`, pending=0 | Register source readiness; live-empty is not EOF. / Ждать source readiness. |
| Partial progress or `AGAIN`, pending>0 | Keep the writer; watch transport writability and finish the same staged bytes. / Дописать тот же остаток. |
| `DVM_IO_RECORD` | The complete declaration/offer pair was acknowledged; another control writer may use the connection. / Пара полностью подтверждена. |
| Successful EOF | Unregister source readiness and retire the writer. CLOSED was completely acknowledged first. / CLOSED уже передан, завершить writer. |
| Failure/uncertain/invalid acknowledgement | Stop using this writer and retire the affected transport according to host policy; never replay an ambiguous tail. / Остановить writer, не повторять неопределённый tail. |

Serialize **all** outgoing control traffic through the staged pair. Do not allow
another producer to interleave bytes after a short write. A zero-byte successful
write for a positive request becomes `NO_PROGRESS` and stops the writer; it is not
an invitation to spin. A count beyond offered bytes, missing PREFIX, unknown
flags, write-EOF or invalid status/stop combination is refused. A confirmed prefix
plus a permanent failure is returned precisely, but cannot be retried through
this writer. Invalid acknowledgements have no trusted byte count.

При частичной отправке используется собственная копия полной пары declaration/
offer. Если source закрылся посреди READY, writer сначала заканчивает именно эту
READY, затем передаёт актуальный CLOSED. Нельзя определять EOF по более новому
snapshot до дописывания старой записи. При ошибке writer становится sticky failed;
проверка его состояния больше не вызывает транспортный callback. Новое соединение
требует нового допуска и нового writer, без переноса старого pending tail.

## Queue and record behavior / Очередь и записи

The existing native queue bounds pending invalidations to32 entries plus one
retained head. Overflow becomes latest-state invalidation; the reader formats
current source state. Intermediate generations may coalesce and identical latest
announcements may repeat; the receiver already handles these monotonically.
There is no unbounded list of historical resource trees. Unpublished reservations
produce no READY and do not signal announcement readiness. Subscribe-before-
snapshot closes the initial publication race.

The low-level `StyxEvents` API returns **one complete declaration/offer pair** per
read. A short output buffer preserves the encoded head; EMPTY means defer/AGAIN.
It does not finish itself after CLOSED; low-level users may call
`styxeventsfinish` and drain normally. The higher-level writer recognizes CLOSED
from its exact staged bytes and returns EOF only after full acknowledgement.
The existing `/dev/sysevent` queue and its event format are unchanged.

For a Styx control file, an empty writer maps to `DVM_IO_AGAIN`. Its server owner
uses original libstyx `styxdeferread` and registers outbox readiness with its
native event loop. Only that owner may complete deferred requests. The producer
passes no server request pointers between threads. Rflush/fid teardown retires
the request before releasing its control context; the source subscription is not
consumed by cancelling one empty read.

## Acceptance / Проверки

- [`tests/writer.c`](tests/writer.c): initially unpublished source, native readiness,
  independent recipients, short output buffers, zero/partial AGAIN, partial READY
  followed by CLOSED, confirmed EOF, malformed/uncertain/permanent failure with no
  replay, overflow/latest state, concurrent publication and repeated detach.
- [`tests/automatic.py`](tests/automatic.py): real original-libstyx TCP with an
  independent Python CRC/integer/9P oracle. A pending Tread does not block another
  Twalk; Tflush cancels it, a reissued read receives generation2 from a native
  producer, then CLOSED3 and EOF, without a client mutation request or guest loop.
- The existing34 actual FlashVM interpreter/JIT TCP scenarios now read the same
  production writer, including three-byte fragmentation and scope change during
  acquisition. Framing and admission tests remain enabled.
- The disconnect scenario deliberately suppresses receiver notice delivery through a
  test-only link wrapper. Availability and fetch still refuse from authoritative
  local connection state, without waiting for that queued notice.
- All five movie suites and31 existing event/subscription regressions pass normally
  and under ASan/UBSan; LeakSanitizer disabled.

The embedding host still owns authenticated connection admission, tag reservation,
transport event-loop scheduling and arbitration with other outgoing S-record
producers. Automatic external FlashVM process creation and a complete distributed
launcher remain subsequent integration work. This change supplies the autonomous
movie-output component for that host, rather than replacing its control protocol.
