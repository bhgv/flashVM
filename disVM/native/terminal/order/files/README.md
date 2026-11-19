# Unibus record files / Файлы записей Unibus

`styx_ordered_files` projects an already granted client of the
[ordered broker](../broker/README.md) as an ordinary **single VFS file**.
Dis applications can use existing `Sys.open/read/readn/write` and FD groups;
no new builtin, init signature, specialized remote application client or display
context is required. The provider can also be published through ordinary VFS/Styx
preparation, mount and bind rules. Binding one such file does not replace its
parent directory or its unrelated siblings.

Это файловое представление одного клиентского канала, а не сырой транспорт.
Имя в namespace выбирает владелец подготовленного VFS. За именем находится
обычный provider с правами чтения/записи и стандартным lifetime открытых FD.
Native-брокер продолжает обслуживать соединение в своих отдельных потоках.

## Bytes and boundaries / Байты и границы

| Operation | Contract |
|---|---|
| Read | Original inner S-record with the fixed granted tag and opaque payload |
| Short read | Copies only that record's prefix; its suffix stays in this open FD |
| Completed read | Sets existing `DVM_IO_RECORD`; `Sys.readn` stops before the next record |
| Write | Exactly one complete original S-record with this file's granted tag |
| Wrong tag, CRC, truncation or trailing bytes | Refuses before queue admission/ordinal assignment, exact zero prefix |
| Empty payload | A nonempty encoded S-record, never confused with EOF |
| Successful write | Whole record admitted once; not remote delivery or execution acknowledgement |

Nested S-record bytes inside a payload remain opaque. Application-local tag0
stays inside its negotiated transport envelope. The file exposes no sequence
prefix, outer lane, native services or transport root0. Empty writes are no-ops.
Concatenated records or a record split across multiple writes are deliberately
rejected: assemble a complete record before each write. This is a record-device
write contract, not a second byte-stream framing negotiation.

Several open FDs for the same client compete for its one inbox; they do not
broadcast records. Readiness and dequeue are atomic. Each open retains at most
one staged record, with its own offset; closing it discards any unread suffix
without replay. Use a single consumer when completion order across separate FDs
matters. A short read never consumes a prefix of the following record.

Clean peer EOF is returned only after ready records and this open's staged bytes
are drained. Explicit owner stop and transport faults report failure. A120000ms
native receive/send deadline does not mean EOF; receive timeout reports AGAIN.
Existing sequence-engine rules still decide which received records are ready.

## Authority and namespace / Полномочия и namespace

[`styxorderedfile`](files.h) accepts only an existing grant and a nonempty subset
of `FVM_ACCESS_READ | FVM_ACCESS_WRITE`. It exposes only dot, with mode0444 and/or
0222. No execute, seek, directory listing, root-control command or arbitrary tag
selection is supplied. Readonly publication refuses writable opens. Later view
policy and actual operations can further restrict access.

**The host must keep raw transport endpoints out of the guest-visible tree.**
A safe file handle cannot hide some other raw carrier grant already published in
that namespace. Use the ordinary default-deny `DVM_View` preparation to publish
only authorized client files/resources before calling init. The original captured
channel descriptor remains a separate native capability. This provider does not
silently edit an existing namespace or revoke unrelated grants.

В проверке настоящего запуска host готовит отдельный обычный view с `/seven`,
`/nine` и `/zero`. Исходный transport-узел `/records` в него не включён; guest
проверяет, что открыть его нельзя. Root0 обслуживает native-брокер, а не Limbo.
Для обычных файлов, каналов, устройств и иных ресурсов применяются те же
существующие правила подготовки выдаваемого дерева и разрешения имён.

## Cancellation and ownership / Отмена и владение

The current native FD operation's cancellation source is passed into cancellable
client waits. Only that operation wakes/refuses; cancellation consumes no record
or ordinal before admission and never stops the broker or another client. Native
callbacks contain no guest addresses, execute no guest code and detach through a
lifetime fence before the borrowed owner can retire. No cooperative guest loop,
forced guest pause or transport polling is introduced.

The new native APIs are `styxorderedclientwaitcancel`,
`styxorderedclientnextwaitcancel` and `styxorderedclientsendwaitcancel`.
The combined receive avoids a readiness/dequeue race between competing opens.
Pre-cancel and subscription allocation failure preserve the head and TX ordinal.
Previously admitted effects are never automatically replayed after cancellation.
Existing noncancellable APIs retain their behavior and numeric results.

`styxorderedfiles(peer)` consumes the broker on every outcome and returns an
explicit native lifecycle owner. `styxorderedfile(files, tag, access)` returns an
owned filtered provider; normal provider-wrapper pins keep it alive through open
FDs. Dropping a factory or closing an FD never stops/joins the shared broker.
The host retains the file-set owner until all exported factories, namespaces,
publication tickets and open handles have retired. `styxorderedfilesfree` returns0
without changing ownership if exports remain; otherwise it joins/frees the broker
and returns1. This keeps blocking joins out of provider destructors. An explicit
`styxorderedfilesstop` wakes all clients without joining. Set operations and final
owner disposal must not race; the host retires users before final free.

```c
StyxOrderedFiles *files = styxorderedfiles(peer); /* Consumes owned peer even on failure. */
/* Check files, then create only authorized publications. */
DVM_Provider *port = styxorderedfile(files, 7, FVM_ACCESS_READ | FVM_ACCESS_WRITE);
/* Check port; map it into an ordinary prepared view, using source path dot. */
/* Drop caller's port ref after the view retains it. */
/* After the view/namespace and every open FD retire, require styxorderedfilesfree(files) == 1. */
```

## Actual coverage / Реальная проверка

The native test uses original libstyx on explicit bidir PHY with three-byte reads
and two-byte write acknowledgements. It checks filtered grants, readonly refusal,
application0 privacy, byte-by-byte reads, empty payloads, malformed/foreign writes,
selected RX/TX renewal and operation-local sticky/active cancellation. Native
condition barriers select cancellation admission; allocation failure is injected
only on the waiting caller. A condition-fenced stop test delays sequence-engine
close after the broker has published retirement and requires the waiting client
to finish without sleeping after that wake. Factory/FD pins prevent early disposal.

The optional actual guest test prepares/takes a real remote-root launch ticket,
attaches the broker from its captured handoff, prepares a filtered application VFS
and invokes the production runtime's pre-init adoption. The same compiled portable
Dis image uses ordinary Sys FDs in both interpreter and JIT modes. The original
server verifies outgoing client records and returns a receipt; after guest shutdown,
FD/provider/namespace ownership drains before native broker/physical teardown.
It needs no display or Unibus builtin callback. The SDK test may link sanitizer
instrumented SDK libraries to the explicitly selected runtime; that does not claim
that an uninstrumented external runtime is itself sanitizer-instrumented.

Select already source-matched built runtime/compiler explicitly with
`INFERNO_DISVM_RUNTIME` and `INFERNO_LIMBO_COMPILER` to register
`terminal-ordered-files-guest`. Normal SDK builds do not download or build a VM
implicitly. C++98 public-header coverage remains a separate test.

This completes a generic file/FD ingress for an explicitly prepared launch. It
is not a public authenticated network launch service, arbitrary FD wire transfer,
remote function-call schema, SWF execution or a default desktop CLI mode switch.


The actual guest now also loads an application and dependency from filtered
execution-provider files served by original Styx, then continues its bidirectional
record exchange on the same connection. The terminal checks a guest startup record
after the code and dependency loads and acknowledges it before the guest continues.
The final receipt separately confirms application output.

Терминал получает стартовую запись гостя после загрузки кода и зависимости и
подтверждает её перед продолжением обмена. Отдельное итоговое подтверждение
относится к выводу приложения.

See the [runtime loader contract](../../../../runtime/PROVIDER-MODULES.md)
for execution-role admission, opened-handle metadata, cancellation and cache isolation.
