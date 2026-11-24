# IDE on the bidir terminal / IDE через bidir-терминал

The existing IDE already separates Dis-owned document/controllers from SWF/AS
presentation and carries copied UI values through ordinary typed channels. Its
`Uibridge`/`makeUiBridge` codec uses the portable `fvd1` envelope; no drawing pointer
or Dis heap address is part of that envelope. This is the reusable basis for the
new terminal architecture, rather than a reason to rewrite IDE commands or forms.

The stock bidirectional launch attaches the initial Dis process's
`$Unibus` to the player's `styxTerminal`. Ordinary Limbo channels carry
complete fvd1 frame bytes. The hosted libstyxrpc dispatcher carries them
over the existing attachment's `.unibus` service.

При штатном двунаправленном запуске `$Unibus.attach` подключает обычные
каналы Limbo к объекту `styxTerminal` в загруженном SWF. Кадры fvd1
передаются через `.unibus` на той же сессии libstyxrpc.
[Штатный плеер](../flashVM/doc/STYX_RPC_PLAYER.ru.md)

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

The two sides register the existing fvd1 envelope under
`_inferno.fvd1` version 1 and grant one frame at a time with
`_inferno.credit` version 1. The tags follow ordinary per-direction
UniBus binding rules. The Dis controller copies bytes between the native
mailboxes and the exact managed channel roots on its turn. The FlashVM
owner materializes each reply in its own context. Detach and process
retirement close the port and release retained channels.

Paired startup reports an empty selected path, no new-document override,
and an initial disabled line-number preference. `rows`, ink operations
and `attachbulk` still require their service bindings. The complete
desktop SWF also needs to apply the desktop's view records and publish
native input through the adapter.

## What stays / Что сохраняется

Existing Dis document/input/workspace services, AS presentation functions and the
local typed-channel path keep their interfaces. Captured pointer motion can still
be handled on the FlashVM owner; the new transport does not require a Dis round
trip for every sample. User files/devices/channels still use VFS; executable
resources retain the agreed provider-first lookup policy.

The graphical `disvm-editor` continues to supply its local UI binding.
The paired player supplies the transport capability to the uploaded SWF.
The complete desktop scene and its remaining host services are tracked
separately from the verified paired terminal example.

## Verification / Проверки

[`terminal-tests.js`](as/terminal-tests.js) loads the actual AS adapter, original
UI codec and presentation helper under Node for algorithm tests: ordinary/missing
capabilities refuse, pressure is not acceptance, two records are received in two
calls, signed values and Unicode/NUL survive, and the existing presentation factory
attaches. The optional IDE CMake checks now include this test and AS parser input.

The stock-pair integration runs [native_terminal.b](../lib/styxrpc/tests/flashvm/native_terminal.b)
and an uploaded SWF in interpreter and JIT modes. It checks exclusive
attach, a synthetic key frame reaching the Limbo input channel, delivery
of a Unicode reply and a second frame acknowledging it. The windowed
run also checks pixels after that reply moves the clip. The SDL tests use
a dummy video driver.
