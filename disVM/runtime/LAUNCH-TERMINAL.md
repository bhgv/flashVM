# FlashVM channels at launch / Каналы FlashVM при запуске (runtime0.168)

Runtime0.169 adds [typed application init and capability FD opening](TERMINAL-INIT.md).
The native helper now accepts `rw` as well as `r+` and sends provider modes
`r/wo/rw` without creation/truncation. Original0.168 integration limits below
are superseded only as described in that follow-up.

The terminal argument supplied to a module's `init` represents the existing
FlashVM communication channel, or a receive/send pair. A provider uses its local
FlashVM through the same channel model. Moving the FlashVM endpoint to the
user's terminal changes routing, not the application interface or graphics API.
Extra bulk/stream channels can be offered through the control exchange.

Терминальный параметр `init` — связь с FlashVM: один двунаправленный канал либо
пара вход/выход. Для локальной и удалённой FlashVM используется один интерфейс.
Дополнительные каналы больших/потоковых данных выдаются через ту же связь.
Рисование, шрифты и устройства остаются за FlashVM и обычными VFS-провайдерами.

## Implemented boundary / Реализованная граница

[`terminal.h`](vfs/launch/terminal.h) adds a native channel capability to the
[launch request](LAUNCH-TICKETS.md) and [admission service](LAUNCH-SERVICE.md).
It is a pinned provider root plus copied relative receive/send names. The root
may be one duplex node (`receive = send = "."`) or a live channel namespace with
separate names. Names in this document are examples, not reserved device paths.
The native launch structures grow; SDK consumers must rebuild and initialize
request specifications to zero before setting fields.

| Stage | Channel ownership |
| --- | --- |
| Submission | `spec.flashvm` is optional and borrowed only during creation; paths are copied and the provider is retained |
| Provider review | `description.terminal_channels` reports presence without exposing the channel grant |
| Preparation | Ordinary isolated VFS preparation; no speculative channel open/stat, traffic or guest execution |
| One-use take | Provider ownership moves into `handoff.flashvm` together with the prepared environment |
| Request/service retirement | Already transferred channel authority survives |
| Handoff disposal | Releases its channel provider reference; independently opened handles keep their own provider references |

The immutable descriptor requires an anchored root and native PATH cancellation.
Grant establishment and association with the authenticated launch session are
the trusted transport adapter's responsibility. Neither channel names nor the
presence bit authenticate a peer or grant permissions. This public native SDK
contains native pointers and must never be serialized as a structure.

В handoff передаётся отдельная capability каналов, а не путь, который затем
ищется заново в union. Перестановка пользовательского окружения не подменяет
выбранную FlashVM. Доступ внутри выданного channel root контролирует провайдер;
отказ не вызывает поиск другого сервера или файла на хосте.

## Opening channels / Открытие каналов

```c
DVM_VfsHandle rx = {0};
DVM_VfsHandle tx = {0};
FVM_Result result;

/* handoff remains owned and live throughout these calls. */
result = dvm_terminal_channel_open(&handoff->flashvm,
    handoff->flashvm.receive, "r", &rx);
if (result != FVM_OK)
{
    return result;
}
result = dvm_terminal_channel_open(&handoff->flashvm,
    handoff->flashvm.send, "w", &tx);
if (result != FVM_OK)
{
    dvm_vfs_close(&rx);
    return result;
}
/* Use normal VFS byte I/O, then consume both handles. */
dvm_vfs_close(&tx);
dvm_vfs_close(&rx);
```

For one duplex endpoint, open its name once with `"r+"` and use that one handle
for both directions under the normal handle ownership/serialization contract.
For a pair, direction is always from the executing module's perspective:
`receive` receives from FlashVM; `send` sends to FlashVM. The backend determines
whether independent opens share a connection or create independent streams.
The adapter must supply endpoints with the intended semantics.

After an authenticated control message offers another channel name, open that
relative name through the same `dvm_terminal_channel_open`. The live provider
decides availability and permissions on each open. There is no frozen list or
tree transfer. Absolute names, empty components and `.`/`..` components are
rejected; a single `.` names the granted root. Only `r`, `w`, `r+` are accepted;
the helper does not request creation, append or truncation.

This helper performs native I/O only, with no guest callbacks, global stop,
cooperative handshake, retry or VM scheduler pump. A native cancellation scope
is forwarded to the opted-in factory; pre-cancel refuses before opening. The
runtime adapter must use the established process I/O admission boundary, inspect
the returned handle's DATA/CLOSE cancellation capabilities and serialize its
operations. Open success must not be discarded because a later cancel arrived.

## Records and transport / Записи и транспорт

Reuse FlashVM's existing [S-record codec](../../flashVM/doc/SRECORD.md),
direction-local [tag bindings](../../flashVM/include/flashvm/sbindings.h),
[fragment receiver](../../flashVM/include/flashvm/sstream.h) and
[bulk negotiation](../../flashVM/include/flashvm/scontrol.h). Keep TX and RX
schema tables separate. A read may return a fragment or multiple records; byte
counts are not message boundaries. The helper transfers opaque bytes and never
reframes, interprets rendering payloads or embeds native/managed pointers.

S-record framing is not a new Styx message dialect. When channels are accessed
through Styx, the existing Styx client/provider transports channel bytes inside
ordinary read/write payloads. Record parsing and additional-channel negotiation
belong to the established endpoint protocol. No competing codec or graphics
proxy is introduced into DisVM.

## Tests and remaining integration / Проверки и оставшаяся интеграция

The native test uses the real FlashVM S-record codec with binary payloads,
coalesced records and one-byte receive fragments. It exercises both one-handle
duplex and two-handle control connections, independent dynamically offered bulk
traffic, immutable input names, one-use handoff, denial/pre-cancel, path escape
refusal and stream lifetime after service/request/handoff retirement. Existing
allocation-failure tests now carry a channel grant through constructor,
preparation, take and service admission cleanup.

This release implements native capability handoff and channel opening. The test
backend is an in-memory native connection, not an executing FlashVM or a network
listener. Automatic conversion to the application's typed `init` argument,
authenticated network capability transfer, and provider-side module execution
are still integration work. There is no claim of a completed remote launch.

Реализованы передача native capability и открытие каналов. Привязка к typed
параметру `init`, сетевая передача capability и запуск модуля у провайдера ещё
не подключены. Специализированная клиентская программа для каждого модуля
не требуется выбранной архитектурой.
