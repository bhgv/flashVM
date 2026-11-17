# Native terminal to host UI bridge / Мост терминала к host UI

`styx_terminal_bridge` connects the native bidir binding dispatcher to the actual
`DVM_UIBridge` channel pair already used by the IDE host. Both directions use
the existing `fvd1` messages. There is no second terminal reader, guest callback,
presentation policy or new wire schema in this adapter.

## Ownership and use / Владение и использование

Call [`styxbidiruibridge`](bridge.h) with an **owned** `StyxBidirRoute`, the local
endpoint role (`DVM_UI_DIS` or `DVM_UI_UI`), and its independently established
outgoing UI tag. The constructor consumes the route on **every outcome** and
returns an owned adapter or NULL. The record limit must be at least
`DVM_UI_HEADER_SIZE + DVM_UI_TEXT_MAX + 32`, so any valid UI frame fits without
misinterpreting an encoding limit as temporary queue pressure.

`styxbidiruibridgelocal` returns the borrowed real local queue pair. Use the
constructor's local role with the ordinary `dvm_ui_send`/`dvm_ui_receive` calls;
their acceptance and one-message receive semantics do not change. The two native
relays own the opposite role. A UI/AS owner can use `dvm_ui_channels` and the
existing `fvm_extensions_wrap_channel` API on its own context. Wrappers remain
rooted and owner-local; only copied value frames cross the native channel.

The adapter is the route's sole UI consumer. The host still services general
records through the borrowed `styxbidiruibridgeroute` accessor; a bounded general
mailbox must not be ignored. RX binding discovery never supplies the TX tag or
grants file/module access. Both existing namespace grants and endpoint schemas
continue to determine which operations are admitted.

RU: Конструктор принимает route во владение при любом исходе и создаёт свои
локальные каналы. Host получает заимствованный `DVM_UIBridge` и использует прежние
API каналов с выбранной локальной ролью. Мост обслуживает противоположную роль
двумя native-потоками, не обращаясь к кучам VM. AS wrappers создаются и удерживаются
на собственном FlashVM owner. Общие записи остаются у host-диспетчера.

## Pressure, cancellation and diagnosis / Очереди, отмена и диагностика

RX retains one copied message until the local channel accepts it. A full local
queue blocks only that native relay. TX retains one copied local message until
the route's bounded TX queue accepts it; it waits on native completion conditions
instead of polling. Queue acceptance never promises remote execution. A terminal
failure is not retried, and uncertain transport writes are never replayed.

The first relay stop/failure reason is available through `styxbidiruibridgestate`.
`styxbidiruibridgestop` closes the owned local channels and owned route, waking
both relays. Other fids and the shared Styx client remain owned by their host.
`styxbidiruibridgejoin` stops/joins the two relays while retaining local queues,
route and diagnostic copies. After all other endpoint users retire,
`styxbidiruibridgefree` also joins/frees the owned route and releases the queues.
Lifecycle join/free calls must be serialized by their host owner.

`styxbidiruibridgepending(adapter, outgoing, &message)` copies a retained record
without consuming it (`outgoing=0` RX, `outgoing=1` TX). On no record/error it
preserves output. A live snapshot may race destination acceptance and therefore
is **never** a replay instruction. Inspect stable pending state after explicit
join while the adapter is still alive. Already queued local/route items and
unaccepted relay copies are distinct; inspect each before deciding recovery.
Explicit stop may abandon unsent work, as with the underlying terminal route.

RU: При backpressure native-поток удерживает одну копию до принятия получателем.
Stop закрывает только собственные очереди и route, затем join завершает оба
потока моста. Общий Styx client сохраняется. Pending-копия доступна для диагностики
после join до free. Живой snapshot может совпасть с уже принятым сообщением и не
даёт права на повторную отправку. Подтверждение очереди не равно исполнению.

## Building and acceptance / Сборка и проверки

The adapter archive references the existing host UI API. An embedding links
`styx_terminal_bridge`, the actual `disvm_flashvm_host`/`disvm_ui_frame` libraries
and its real FlashVM target/dependencies. The Inferno `lib9` and `libstyx` targets
gain no VM, graphics or font implementation. No FlashVM build/download is
started implicitly by this native adapter target.

Native acceptance tests are enabled only when `INFERNO_UI_HOST_LIBRARIES` supplies
the explicit real host/FlashVM library list. They reuse the actual launch handoff,
libstyx/lib9 peers over a fragmented PTY, actual FlashVM channels and existing AS
channel wrappers. The fixtures cover both local roles, local RX backpressure,
short remote writes, repeated replies, undersized routes, malformed remote UI,
local close and a deterministic TX-admission failure preserving the pending copy.
The TX failure is a test-only link-wrapper injection, not a simulated PHY driver.

[`tests/bridge_as.c`](../tests/bridge_as.c) invokes the real AS wrapper methods on
their FlashVM owner. A test-only native gate holds the copied outgoing frame
while the original wrappers/context are destroyed; only then is the real sender
released. The provider checks exact bytes and returns a fixture-only receipt.
This verifies copied ownership across context retirement, not execution of a
SWF application or a complete remote IDE.

The host channel boundary is connected. Automatic runtime `$Idehost` attachment
and remote rows/path/ink/bulk RPC endpoints remain separate integration work.
