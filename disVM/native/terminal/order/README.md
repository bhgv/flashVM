# Ordered Unibus packets / Упорядоченные пакеты Unibus

This is an **explicit data-lane profile**. Existing connections carrying plain
S-records retain their framing. A host must agree this profile at both ends
before using it; it is not silently inserted into `styxscope` or FlashVM channels.
The codec is shared by the standalone C library and FlashVM itself. No graphics,
VM heap pointers, TCP or UDP assumptions enter the format.

Это явный профиль линии данных. Его применение согласуется обоими концами;
существующие каналы обычных S-записей не меняются автоматически. Кодек общий
для FlashVM и отдельной C-библиотеки, без зависимости от графики или носителя.

## Exact wire format / Точный формат

Each packet is a compact sequence prefix followed by **one complete original
S-record**, including that record's existing checksum. The prefix is outside
that S-record. Nested application S-records do not acquire prefixes implicitly.
The native broker chooses which namespace/tag/direction has an ordered lane;
applications receive their own copied payloads, not transport control authority.

| Prefix part | Encoding |
|---|---|
| Header bits 2..0 | Number byte count minus one: 0 means 1 byte, 7 means 8 |
| Header bits 7..3 | CRC-5 over the three length bits followed by the number bytes |
| Number | Unsigned, big-endian, minimal 1..8 bytes, no continuation/sign bits |
| Zero | One zero number byte; leading zero bytes in longer numbers are invalid |

| Number | Complete prefix, hexadecimal |
|---|---|
| 0 | `40 00` |
| 255 | `e0 ff` |
| 256 | `99 01 00` |
| 18446744073709551615 | `ff ff ff ff ff ff ff ff ff` |

Total prefix size is **2..9 bytes**. There is **no generation field in each
packet**. Example canonical prefixes: see the independent polynomial oracle in
[tests/oracle.py](tests/oracle.py), which checks all byte-width boundaries and
4096 deterministic random uint64 values against the C implementation.

CRC profile: polynomial **x^5 + x^2 + 1** (`0x05` without the leading term),
initial register `0x1f`, final XOR `0x1f`, MSB-first input, no reflection or
padding. Feed exactly three bits of `count - 1`, highest bit first, then each
number byte MSB first. This explicitly defined profile is not labelled USB CRC.
CRC-5 is an accidental-corruption check; it is not authentication and does not
cryptographically bind a valid prefix to a valid S-record body. The original
S-record checksum remains mandatory. There is no guessed byte resynchronization:
a damaged lane requires its owner to recover an independently known boundary or
retire that lane/session.

Перед S-записью идут один байт длины/CRC и 1..8 байт беззнакового номера,
старшими байтами вперёд. Ноль занимает один байт номера. Ведущие нули запрещены.
CRC проверяет три бита длины и байты номера; CRC исходной S-записи сохраняется.
Автоматического угадывания границы после повреждения нет.

## Portable codec / Переносимый кодек

Public header: [sorder.h](../../../../flashVM/include/flashvm/sorder.h).
The public C89/C++98 ABI represents unsigned64 as two checked uint32 limbs,
`high` and `low`; native struct layout is never serialized.

- `fvm_sordinal_write/read`: prefix only; outputs change only on success.
- `fvm_sordered_write/read`: prefix plus one original verified S-record.
  A full read leaves following packets unread; a refused write changes no output.
- `fvm_sordered_stream_init/push/reset`: caller-buffered incremental decoder,
  at most one record per successful push. `used` reports the actual consumed
  prefix on valid calls. Both complete views and errors stay latched until reset.
  Borrowed views expire when their input/storage is changed or reused.

Buffers/state/views must be disjoint as specified by the public contracts.
Body limits include the original encoded S-record tag. Prefix bytes are separate.

## Native broker queues / Очереди native-брокера

[order.h](order.h) owns one namespace's independent `(tag, direction)` counters.
Default first number is zero. `styxorderstart` can seed an **unused** direction,
including near MAX for testing or an explicitly agreed session checkpoint.
It cannot rewrite a live counter. Use separate engines for separate namespaces:
identical numeric tags in different client envelopes do not share state.

`styxorderencode` assigns a TX number only after successful complete encoding.
The native owner must retain the returned packet and retry **those same bytes**
until transmission succeeds or the connection is retired. Encoding a retry as a
new packet would allocate another ordinal. The engine does not perform I/O or
own a worker: the existing native channel broker owns reception/transmission.

`styxorderfeed` validates exactly one full packet and copies its payload. Future
records wait in that tag's sorted held list until gaps close; other tags remain
readable. Identical held duplicates and old ordinals are not delivered twice.
Different payloads claiming the same **held** ordinal produce a conflict; payloads
already admitted to the ready queue or consumed are not retained as a replay
history. The configured window bounds future ordinal displacement, not the ready
application queue length. There is no fixed application-message capacity here;
allocation failure is explicit and does not discard another queue's records.

`styxordernext` returns **one** selected-tag payload. A small output buffer only
reports required size; it does not consume the record or advance its checkpoint.
`styxorderwait` blocks only that native consumer, using a monotonic condition
budget, never a guest pause or cooperative polling. Closing wakes waiters and
leaves already-ready records individually drainable. Free only after all callers
have retired. A missing-gap held record cannot become ready merely due to close.

Брокер восстанавливает порядок отдельно для каждого тега. Пропуск одного тега
не блокирует другой. Клиенты получают по одной записи и ждут только свою очередь
на native condition. Маленький буфер не потребляет запись. Готовые сообщения
сохраняются при закрытии; записи за незаполненным пропуском не выдаются.

## Selected-counter renewal / Сброс выбранного счётчика

No implicit wrap occurs. At MAX only the exhausted counter refuses further
encoding. At **one billion records per second**, uint64 covers about **584.5
years**; the protocol still defines renewal so correctness does not depend on
that being distant. Renewal may also be requested before MAX.

1. Spool the selected tag's last old packet. Its native owner queues a service
   request **after** those old packets, identifying the selected namespace, tag,
   direction and final old ordinal. Unrelated tags continue normally.
2. `styxorderprepare` reserves that one directional counter using a strictly
   increasing nonzero **local operation ID**. TX checks its last spooled ordinal.
   RX accepts old packets up to the proposed boundary; it refuses a crossing.
3. The receiver restores the old order and clients consume the final old record.
   `styxorderready` requires that consumed boundary and empty held/ready queues.
   Merely decoding, holding or queueing a record does not complete this barrier.
4. The native service adapter sends the corresponding ACK. Each side applies
   `styxorderconfirm` to its selected TX/RX direction only. Exact latest duplicate
   confirmations are idempotent; wrong IDs/results or not-ready RX confirmations
   do not change state. An explicit NACK releases the proposal without resetting
   the old counter. A timeout never implicitly resets or releases it.
5. New selected-tag data starts at zero after accepted confirmation. Other tags,
   the opposite direction, and other namespaces retain their counters.

Before accepting renewal, the adapter must retire all possible old deliveries,
including delayed duplicates/retries on additional lanes. **A number alone cannot
separate an old zero from a new zero after reset.** Local operation IDs are not a
substitute for the session/transport barrier. A reconnect must establish a new
agreed session and retire old lane traffic, rather than reusing an ambiguous
connection. Do not destroy the old replay boundary while any old lane can deliver.

The low-level native APIs do **not** assign numeric wire command IDs, authenticate peers,
provide automatic request retry, or install automatic striping/carrier growth.
The existing [root service hook/outbox](../scope/operation/README.md) can carry
an adapter's negotiated control messages on an ordinary ordered S-record control
path, independently of explicit ordered data lanes. An adapter must resolve
cross-lane boundaries before committing assignments. The real-Styx fixture's
plain tag0 `release` command is test-only, not a production reset schema.

Сбрасывается только выбранный счётчик после request/ACK. Остальные продолжают
работу. Старые доставки и повторы должны быть завершены до сброса: без поколения
в каждом пакете отличить старый ноль от нового невозможно. Этот шаг выполняет
native-адаптер сессии. Формат его служебных команд здесь не назначается произвольно.

## Verification / Проверки

- Independent polynomial-division oracle, all widths, truncations, data/CRC
  bit corruption, original body checksum, output preservation.
- Every packet split point, bytewise reception, coalesced tail, sticky error/reset.
- Independent tag gaps, duplicate/conflicting held records, reorder window,
  copied payloads, zero-length payload, small buffer, native wait and close/drain.
- MAX-1/MAX arrival reversal, no automatic wrap, consumed ACK barrier,
  selected-direction reset, unrelated TX continuation, NACK, stale/duplicate IDs.
- Original libstyx server and client over real explicit bidirectional PHY pumps,
  three-byte reads, deferred idle Tread and an independent native Twrite while
  the broker is receiving. A client reads tag9 while tag7 still lacks number0.
- Portable prefix endpoint compiled as C90; public API linked by a C++98 caller.

The ordinary complete native suite and ASan/UBSan suite cover the existing
plain-S connections too. These checks do not claim automatic ordered-profile
negotiation or a completed multi-lane scheduler.

## Integrated broker / Подключённый брокер

The explicit [negotiated bidirectional broker](broker/README.md) now supplies
the native service adapter, named OFFER/ACCEPT exchange, retained packet outbox,
per-client capabilities and automatic selected-counter renewal on one original
Styx stream. It keeps ordinary outer S-record framing and carries this prefix
inside the negotiated lane envelope. Low-level raw ordered lanes remain available.
Existing flat/scoped/FlashVM launch profiles are not silently upgraded.
