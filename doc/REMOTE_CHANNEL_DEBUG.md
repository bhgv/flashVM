# Remote channel debugging v0.67 / Сетевые каналы в отладчике

`fvm_debug_channel_freeze` и `fvm_debug_channel_state` поддерживают network
proxy каналов, экспортированных совместимым FlashVM broker. Freeze изменяет
состояние authority: local и remote readers останавливаются, writers продолжают
отправку. Resume разрешает receive; ожидающий remote worker опрашивает сервер
неблокирующими запросами и может быть возобновлён другим клиентом.
Сервисный цикл брокера никогда не ждёт данные в frozen канале.

State возвращает pending/capacity/closed/frozen из одного snapshot authority.
Поле identity обозначает локальный proxy handle, а не remote native ID.
Счётчики в control wire ограничены Uint32. Числа кодируются big endian без
зависимости от host alignment, size_t или endian. Ответ проверяется полностью
перед изменением caller output; short/null/invalid flags и transport/OOM ошибки
не меняют output. Freeze требует пустой acknowledgment.

В FVMC protocol v1 добавлены операции 7 (one-byte boolean freeze) и 8
(12-byte state). Старые brokers отвергают новые операции; negotiation отсутствует.
Bearer capability URI использует существующие правила доступа. Для C debug API
без context ошибки транспорта фиксируются с URI и context=0; это не заменяет
ActionScript exceptions обычных send/receive с полным script context.

Authority может быть queue или file channel. File snapshot считает complete
framed records, не decoded graphs, и сохраняет read position. Локальный/remote
file snapshot ограничен 65536 records и 64 MiB scan; превышение оставляет output
и cursor неизменными. Файл, изменяемый извне, не замораживается. Remote closure
отвергает receives и freeze/resume, даже если закрытая native очередь ещё
содержит данные; state разрешён до остановки брокера. После disconnect state
возвращает failure и не публикует частичный snapshot.

Открыто: remote indexed message peek, source-aware remote waiter snapshots,
распределённый deadlock graph, negotiation/authorization granularity, latency
и длительная межпроцессная приёмка. Стадии 6/7 не закрыты этой правкой.

Проверки: channel_debug_network (loopback TCP queue+file, shared freeze,
writer progress, blocked worker/resume/close, disconnect/output rollback);
channel_debug_protocol (short/null/invalid flags, OOM/NOT_FOUND, invalid ACK);
file_channel_debug (bounded scan/output/cursor rollback). На profiles без
OS_PORTS сетевой тест исключён preprocessor guard; это не network certification.

## English

Compatible brokers now support authoritative freeze/resume and coherent state
snapshots through remote channel proxies. Frozen reads do not block the broker
service loop; producers remain active and waiting workers poll nonblocking
receives. State reports authoritative pending/capacity/closed/frozen; identity
is the caller's local proxy identity.

FVMC v1 controls 7/8 use one-byte freeze input and a 12-byte big-endian state
response with Uint32 counters. Invalid/short/null replies and transport/OOM
failures preserve caller output. Freeze requires an empty acknowledgment.
Older brokers reject these controls; version negotiation is not implemented.
Debug APIs without context report transport URI with context zero; ordinary
ActionScript resource operations retain their existing exception context.

Queues and local file authorities are supported. File scanning preserves its
cursor and is bounded to 65536 framed records/64 MiB; decoded graph validity and
external file mutations are outside that snapshot contract. Closed authorities
still provide state but reject remote receives and freeze/resume. Disconnection
prevents snapshot publication.

Remote message inspection, source-aware waiters, distributed deadlock graphs,
negotiation and sustained independent-process acceptance remain open. Tests cover
loopback queue/file freezing, writer progress, worker resume/closure, disconnect,
malformed control responses and bounded file rollback. OS_PORTS-disabled profiles
do not exercise live network behavior. Stages 6 and 7 remain open.

Проверено / Verified: full headless 243/243, AGG 259/259, Cairo 259/259, portable 234/234; targeted ASan/UBSan 9/9. Extended worker wrapper checks additionally pass after the full runs: RESOURCE_CLOSED, manually assigned source network-worker.as:42 and network URI survive the exception path. Portable OS_PORTS-disabled network test is compiled out. Leak detection/stack ASan remain disabled; real Windows/macOS/Android execution is not certified.
