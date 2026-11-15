# Chunk results and failure-only metadata / Чанки и metadata после отказа

Copyright (c) 2026 bhgv

Runtime 0.128 implements native precise I/O outcomes and the accepted rule:
**perform the operation first; query metadata only after failure**. This is an
internal provider/runtime interface. No new Styx messages, ping/pong transport,
guest Sys signatures or union file switching modes are implemented in this step.
Runtime 0.128 реализует precise native I/O result и правило: сначала операция,
metadata только после отказа. Это внутренний API, не подключённый Styx transport.
Новых Sys signatures и режима переключения открытых union файлов здесь нет.

## Implemented interface / Реализованный интерфейс

[`DVM_IoResult`](vfs/io_result.h) separates acknowledged prefix count, original
operation status, stop cause, uncertain tail and diagnostic metadata status.
`DVM_VfsOps.read_chunk/write_chunk` are optional precise native callbacks.
[`dvm_vfs_read_chunk/write_chunk`](vfs/handle.h) execute once and record the outcome
in the handle; `dvm_vfs_last_io` copies it under the same caller FD pin/FIFO.
Legacy callbacks remain source-compatible. Their successful write acknowledges
all requested bytes; a legacy failure does not reveal its actual partial count,
so the adapter marks effects uncertain instead of inventing an acknowledgement.
DVM_IoResult разделяет confirmed prefix, status, причину остановки, uncertain tail
и diagnostic metadata. Optional precise callbacks сохраняют count даже при отказе.
Legacy failure не сообщает actual count: он помечается uncertain, а не превращается
в подтверждённый ноль. API внутренний, не stable native DLL ABI.

| Outcome / Результат | Runtime behavior / Поведение |
| --- | --- |
| Success, including short success / Успех, включая короткий | Return actual count, no diagnostic stat / Actual count, без diagnostic stat |
| Read EOF, including a positive final prefix / Read EOF с возможным prefix | Return confirmed bytes; readn stops; no diagnostic stat / Вернуть bytes, остановить readn, без stat |
| Refusal after a confirmed prefix / Отказ после prefix | Return positive count and preserve native cause; one diagnostic info attempt / Positive count, причина сохраняется, один info |
| Refusal without confirmed progress / Отказ без progress | Sys returns -1; native outcome distinguishes reason/uncertainty / Sys -1, native result сохраняет причину |
| Positive write request acknowledges zero / Ненулевой write подтвердил ноль | NO_PROGRESS failure, not EOF; diagnostic info attempt / Отказ NO_PROGRESS, не EOF |
| Zero-size successful write / Успешный zero-size write | Valid zero, no diagnostic stat / Допустимый ноль, без stat |
| Transport/backend effects unknown / Неизвестные effects | Preserve any confirmed prefix and uncertain tail; no replay / Сохранить prefix и uncertain tail, без replay |

The dispatcher probes the same opened resource through `info`, which maps to
`fstat(fid)`/Tstat for a future Styx provider. A retained wrapper preserves the
inner probe result, preventing duplicate metadata queries. Unsupported info or a
failed diagnostic query does not replace the original I/O result. Observed mode
bits are a hint, not proof of effective access or of the reason for failure.
Dispatcher вызывает info того же opened resource. Для будущего Styx provider это
fstat(fid)/Tstat. Wrappers сохраняют результат, не повторяя запрос. Ошибка info
не стирает I/O result. Mode bits — hint, не доказательство причины отказа/прав.

Ordinary `Sys.read`, `readn`, `write` and `fprint` use these outcomes. A confirmed
prefix is published exactly once. Managed input bytes beyond that prefix remain
untouched. Native callbacks, diagnostics and last-outcome writes run outside
managed admission under the existing descriptor FIFO; no global stop or namespace
lock is added. Last outcome is native-only and is replaced by the next chunk;
it is not a pending exception automatically injected into the next operation.
The ordinary Sys integer result cannot expose an uncertain tail; automatic
transport/layer retries must inspect the native result instead of assuming the
unacknowledged remainder had no effects.
Обычные Sys calls сохраняют prefix ровно один раз; хвост input не меняется.
Callbacks/diagnostics идут вне managed admission под прежним FD FIFO. Last outcome
пока native-only; следующий chunk заменяет его, отложенное исключение не вводится.

The existing MAUTORWX open/create/load permission checks remain selection checks.
This change removes no ordinary authorization and adds no per-success I/O probe.
After opening, the actual operation controls dynamic access. Native POSIX handles
retain their usual rights across chmod; only a provider implementing revocation
can revoke its opened stream dynamically. No permission is fabricated by the host.
MAUTORWX checks при open/create/load сохраняются. Дополнительного stat после
успешного I/O нет. Dynamic refusal задаёт сама операция provider; native POSIX FD
сохраняет свои обычные права после chmod.

## Accepted future transport contract / Принятый будущий контракт транспорта

Styx/9P2000 has Tstat for attributes and Topen for initial permission checking.
An opened fid ordinarily retains access across permission changes; dynamic
per-chunk revocation is an explicit extension, not claimed Inferno behavior.
References: [open(5)](https://www.inferno-os.org/inferno/man/5/open.html),
[read(5)](https://www.inferno-os.org/inferno/man/5/read.html),
[stat(5)](https://www.inferno-os.org/inferno/man/5/stat.html), and the protocol
selection pinned in [UPSTREAM.json](../UPSTREAM.json). The upstream snapshot stays
unchanged. [cat-v](https://doc.cat-v.org/) is an additional reference archive;
protocol versions must be matched before applying its documents.
Styx сохраняет обычный opened-fid contract. Per-chunk revocation — наше явное
расширение; ссылки выше, upstream snapshot не меняется.

For the future provider, successful Tread/Twrite requires no additional Tstat.
A reported failure can trigger one bounded diagnostic Tstat on the same fid,
subject to connection availability, remaining deadline and cancellation. This
query must not authorize or retry the failed operation, erase its count, or turn
an unavailable server into EOF. Rread/Rwrite already carry counts; standard
Rerror does not simultaneously carry a partial acknowledgement plus a structured
cause. A precise provider can therefore return a short confirmed reply and defer
failure reporting, or negotiate an extension; neither is silently assumed here.
Для будущего provider: success без Tstat, failure — максимум один bounded Tstat
при доступном соединении/оставшемся deadline и без отмены. Запрос не разрешает
повторную операцию и не меняет confirmed count. Rerror не содержит одновременно
partial count и structured cause; нужен short reply с последующим отказом либо
явно согласованное расширение.

A future heartbeat may piggyback a provider-wide change hint in its negotiated
message envelope. A monotonically increasing generation within a session is
preferable to a bare bit: duplicated/late hints can be ignored and several changes
coalesced. Reconnection starts a new session; counter exhaustion cannot wrap.
The hint invalidates cached assumptions, but grants no rights and triggers no
per-success Tstat. This step carries optional CHANGED/generation in native chunk
outcomes; no network receiver, cache or heartbeat protocol is claimed implemented.
Будущий heartbeat может нести changed/generation в согласованном envelope.
Generation позволяет объединять изменения и отбрасывать старые hints; новая
session при reconnect, без wrap. Hint сбрасывает cached assumptions, но не даёт
прав и не требует stat после success. В native result есть CHANGED/generation;
сетевого heartbeat/receiver и кэша в этом шаге нет.

Standard Styx message layouts must remain intact: do not append arbitrary bytes
to Rread/Rwrite or introduce unsolicited replies. Negotiate an envelope/extension
or expose notifications through a control/event file. A lost write reply leaves
uncertain effects, especially for channels, devices and append writes: never replay
those bytes merely because a subsequent stat succeeds or another layer is writable.
Не менять стандартные layouts и не вводить unsolicited replies без negotiation.
Возможны negotiated envelope либо control/event file. Потерянный write reply
не позволяет replay на другом слое: effects могут уже произойти.

Union-wide EOF versus denial/temporary failure, bounded traversal, and lower-layer
offset mapping still belong to the future file-stack cursor. Neither concatenation
nor automatic switching of an already opened FD is introduced before those modes
are agreed. EOF of one source is not evidence that an entire future stack ended.
EOF всей стопки, bounded traversal и mapping offsets относятся к будущему cursor.
Режим склейки/переключения открытого FD пока не выбран; EOF одного source не
означает EOF всей будущей стопки.

## Acceptance / Проверки

[Native unit](vfs/test_io_result.c) checks partial failure, unknown legacy effects,
zero-progress/EOF, changed hints without stat, malformed count rejection,
independent probe errors and exactly one nested-wrapper probe.
[Real guest fixture](fixtures/provider-chunk/main.b), [provider](test_provider_chunk.c)
and [driver](test_provider_chunk.py) verify readn/read/write/fprint acknowledgements,
untouched input tails, five failure probes and zero success/EOF probes, exact
callback counts and retained provider lifetime in interpreter/JIT and both polls.
Native unit и реальный guest проверяют counts, причины, отсутствие replay и
лишних probes, input tails и consuming lifetime.
