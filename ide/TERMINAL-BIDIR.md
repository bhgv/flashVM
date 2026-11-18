# IDE on the bidir terminal / IDE через bidir-терминал

The existing IDE already separates Dis-owned document/controllers from SWF/AS
presentation and carries copied UI values through ordinary typed channels. Its
`Uibridge`/`makeUiBridge` codec uses the portable `fvd1` envelope; no drawing pointer
or Dis heap address is part of that envelope. This is the reusable basis for the
new terminal architecture, rather than a reason to rewrite IDE commands or forms.

Основные изменения относятся к engine/host: соединение, VFS, запуск, S-record
диспетчер, владение каналами и завершение. В IDE достаточно небольшого adapter у
канальной границы. Но автоматическое получение terminal capability и маршрутизация
её в `$Unibus` ещё должны быть подключены — одного изменения названия режима мало.

[`Unibus`](../docs/UNIBUS.md) is the universal application interface, not an IDE
execution mode. The [per-launch native binding](../disVM/runtime/host_interface/README.md)
now installs that authority before `init`, with explicit inheritance and lifetime
policies. A wire-backed implementation of its calls still needs the remote
endpoint/marshalling below. All shared terminal traffic stays inside S-records.

## Implemented adapter / Реализованный adapter

[`terminal-bridge.as`](as/terminal-bridge.as) exposes:

- `makeIdeTerminalBridge(terminal)`: the existing `makeUiBridge` codec on a host
  bidir terminal's UI-only send/receive capabilities;
- `makeIdeTerminalPresentation(terminal, target)`: the existing presentation adapter
  attached through that codec, without moving document/window policy.

The host capability declares `bidir === true` and supplies two nonblocking methods:
`trySendFrame(frame)` copies one accepted existing `fvd1` string and returns true;
false means no acceptance/backpressure. `tryReceiveFrame()` returns one routed
UI frame or undefined. The capability remains alive on the FlashVM owner through
all adapter uses. It is an embedding capability, not a native object serialized
over the network. Other terminal types never pass through its UI-only reader.

The [native S-record transport](../disVM/native/terminal/README.md) now supplies
bounded record queues and automatic RX/TX I/O over the prepared bidir handoff.
The native [`styx_terminal_ui` route](../disVM/native/terminal/ui/frame.h) now
encodes/sends and validates copied `fvd1` values using the same codec as local
queues. Its decode helper receives already dispatched records and does not become
a second terminal reader. The host must still bind that route to this FlashVM
capability and attach it to `$Unibus`. Events, resource
loading, timer/render notifications and proxy messages remain independent typed
routes on the same general terminal connection. No new UI wire schema is assigned
by this adapter, and no mandatory AS relay is imposed on unrelated typed calls.

An opt-in [native binding dispatcher](../disVM/native/terminal/route/README.md)
now discovers agreed UI type/version bindings on its own native thread and
provides one-message copied UI/general mailboxes. It consumes no guest execution
turns. The existing `$Unibus` additionally has rows/path/ink/bulk RPCs; their
remote endpoint and runtime attachment are separate remaining integration work.

The [native host-channel relay](../disVM/native/terminal/bridge/README.md) now
provides a real `DVM_UIBridge` endpoint on that route. The ordinary local codec
and actual AS channel wrappers work on it; relay workers exchange only copied
values. Both local roles and context retirement are covered by original 9P peer
tests. This does not yet register a remote `$Unibus` instance or implement its
non-message RPC endpoints.

## What stays / Что сохраняется

Existing Dis document/input/workspace services, AS presentation functions and the
local typed-channel path keep their interfaces. Captured pointer motion can still
be handled on the FlashVM owner; the new transport does not require a Dis round
trip for every sample. User files/devices/channels still use VFS; executable
resources retain the agreed provider-first lookup policy.

The current installed desktop still obtains local ports from `$Unibus` through
its parameterless bootstrap. It is **not yet automatically launched remotely**
by this change. The remaining host integration must provide the bidir terminal
capability, route existing UI frames and retire it with the job. Its owner/queue
logic belongs in the engine; editor behavior remains in executable Dis/SWF modules.

## Verification / Проверки

[`terminal-tests.js`](as/terminal-tests.js) loads the actual AS adapter, original
UI codec and presentation helper under Node for algorithm tests: ordinary/missing
capabilities refuse, pressure is not acceptance, two records are received in two
calls, signed values and Unicode/NUL survive, and the existing presentation factory
attaches. The optional IDE CMake checks now include this test and AS parser input.

These Node checks are not FlashVM movie execution, remote desktop acceptance or
physical input tests. Native bidir/record/handoff execution is tested separately
through original libstyx peers. A complete remote IDE execution remains to be
verified after the host attachment step.
