# Resource replies on Unibus / Ответы ресурсов через Unibus

[`reply.h`](reply.h) exposes the existing `flashvm.movie` version 1 reply receiver
independently of its transport. It accepts one checked S-record **payload** at a
time, after ordinary Styx framing or the
[typed Unibus dispatcher](../terminal/route/README.md). The ordinary
`styxmoviefetch` client now uses this same receiver. No new S-tag, opcode,
file protocol, path authority or physical transport is introduced.

RU: Общий приёмник обрабатывает существующий формат `flashvm.movie` версии 1
после проверки S-записи. Один код используется обычным Styx-загрузчиком и может
принимать payload из typed-очереди Unibus. Новых числовых тегов и протоколов нет.

## Native interface / Native-интерфейс

| API | Contract |
| --- | --- |
| `movie_reply_init` | Initialize caller-owned state and borrow sink/context/output through completion. Invalid arguments leave state and output unchanged. |
| `movie_reply_accept` | Consume exactly one verified resource payload; return MORE, DONE or FAILED. Only valid BYTES calls the native sink. |
| `movie_reply_result` | OK only after successful END; otherwise ERROR or the preserved sink error. |

`MovieReply` belongs to one native owner. No internal worker, allocation, VM
callback, wait or transport read is hidden in it. The sink executes synchronously
on that owner and must stage bytes. The caller supplies total-size quotas and
discards staging on refusal, cancellation, missing END or connection failure.
This receiver bounds each data chunk and the resolved path, not total resource
size. Input, state and output buffers must not overlap.

Use a queue registered with `MOVIE_TYPE` and `MOVIE_VERSION`. Its selected RX
schema is not a TX permission. Feed only RECORD results; EMPTY is no progress,
END is transport termination, and SMALL preserves the queue head. Consume
UI/general/other schemas separately. Resource bytes remain inside S-records.

For a state already initialized with a native staging sink:

```c
unsigned char bytes[MOVIE_CHUNK + 1];
unsigned long tag;
size_t size;
int received = styxbidirroutetypednext(route, resource_index,
    &tag, bytes, sizeof(bytes), &size);
if (received == STYX_CONTROL_RECORD)
{
    int accepted = movie_reply_accept(&reply, bytes, size);
    /* MORE keeps staging; DONE allows commit; FAILED requires discard. */
}
```

The existing resource protocol has no request correlation ID. Each receiver
handles **one active resource reply**; do not mix concurrent requests' payloads.
Use a dedicated granted subchannel or an explicitly serialized service exchange.
General RPC correlation and automatic channel negotiation remain separate work.
A numeric RX tag is not a request ID or a VFS access grant.

RU: Состояние принадлежит одному native owner. Sink вызывается синхронно и
сохраняет staging; публикация и общий quota принадлежат caller. Один приёмник —
один активный ответ. Нельзя смешивать несколько запросов без отдельного канала
либо явно последовательного обмена: в этом формате нет correlation ID.

## Validation and completion / Проверка и завершение

The legal success sequence is BEGIN, zero or more BYTES, then END success.
A failed END terminates an unsuccessful acquisition. BEGIN carries a bounded
canonical absolute virtual origin: no NUL, empty components, `.`/`..` or trailing
slash except root `/`. This checks syntax; it does not normalize names, decide
permissions, reopen the path or interpret it as a host filesystem location.

BYTES contains `1..MOVIE_CHUNK` data bytes. Empty BYTES is malformed, not EOF.
NUL/binary bytes are preserved exactly. Counter overflow, repeated BEGIN, data
before BEGIN, unknown opcode and malformed END refuse. Malformed payloads never
invoke the sink. Sink errors are sticky and never retried.

The output origin remains unchanged until successful END. Partial data, an empty
queue, transport EOF and cancellation do not complete acquisition. Stop feeding
after DONE or FAILED. An extra call after DONE refuses and invalidates receiver
success; it cannot erase a path already published for the prior completed result.
Returned origin text is metadata, not authority.

RU: Успех — только после BEGIN, допустимых чанков и END success. Пустой ресурс
допустим, пустой BYTES — ошибка. Повтор BEGIN, неверный путь, слишком большой
чанк и переполнение отклоняются до sink. Путь выдаётся после успешного END;
доступ определяет исходная операция VFS, а не возвращённая строка.

## Verified scope / Границы проверки

[`tests/reply.c`](tests/reply.c) covers independent binary payloads, ordering,
sticky sink refusal, exact/max bounds, overflow and publication timing.
The [native 9P fixture](../terminal/tests/resource_reply.c) receives that schema
beside UI and opaque traffic over fragmented original libstyx/lib9 PTY peers.
Cases include success, failed END, sink refusal, omitted END and malformed origin.
UI/general traffic and the original session remain usable afterwards.

These fixtures stage native bytes; they do not execute fixture SWF content or
implement the proposed Unibus `file/state` VFS control nodes. Actual managed
movie loading remains covered by the existing movie player acceptance tests.
