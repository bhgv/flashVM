# Captured ordered attachment / Подключение закреплённых каналов

[`attach.h`](attach.h) is the common native construction path used by the binary
[`unibus-bidir` CLI](../../cli/README.md) and the optional
[FlashVM value-channel bridge](../../bridge/ORDERED.md). It needs no display,
guest VM, editor or UI-host library. It opens only the channel descriptor already
granted by the native owner and never chooses resources by backend kind.

Общий native-конструктор подключает ordered Unibus к ранее выданным именам
каналов. Файл, канал, socket или устройство за именем определяет VFS. Приложение
получает только handles своих внутренних тегов, а не transport или root0.

| Native entry | Purpose |
|---|---|
| `styxorderedoptionsvalid(options)` | Pure profile validation before any I/O or worker setup |
| `styxbidirorderedchannels(client, channels, options)` | Construct from a captured channel descriptor and queue initial offer1 |
| `styxbidirorderedlaunch(client, handoff, options)` | Use exactly the channels of an exclusive pre-adoption launch handoff |

All inputs are borrowed through return. Success returns one owned `StyxOrdered`;
failure returns NULL and releases staged native owners. The caller's original
client, provider, handoff, namespace, arguments and unrelated fids are retained
by their original owners. `styxorderedfree` consumes the returned broker after
its borrowed client/service users retire. Freeing it ends that broker, not the
shared Styx session.

## Native profile / Native-профиль

`StyxOrderedOptions` contains exact native `struct_size`, inner body `limit`,
future-displacement `window`, outgoing outer `lane`, and a borrowed unique inner
tag list. The broker copies grants before publication. Native pointer layouts
are never serialized. The incoming lane is independently negotiated.

Limit is1..65472 **including the encoded inner tag**. For example, with limit1,
tag0 can carry an empty payload; a larger tag requiring more bytes cannot be
granted at that limit. Query `styxorderedclientceiling` for an individual local
payload ceiling; a negotiated TX limit may be smaller. Outer capacity reserves64
framing bytes and at least256 bytes for private services, independently of a tiny
application body. Window is1..2^63−1; lane is nonzero unsigned32. Inner0 is valid
and application-local. A zero-count list permits a native service-only owner;
CLI and FlashVM adapters impose their own nonempty/UI-grant requirements.

Invalid descriptor size, limits, tags, duplicate grants and paths refuse before
control construction. Explicit bidir client capability is required. The ordinary
control then verifies the exact captured provider/client association and opens
separate retained RX/TX fids. There is no ambient path search or one-way fallback.
Current native cancellation applies to construction: pre-cancel performs no I/O;
ordinary in-flight acquisitions use their existing cancellation/flush path. Checks
after staging prevent publishing a cancelled owner. Once construction returns,
that temporary scope does not own the broker's lifetime: its native owner calls
stop/free explicitly.

Согласование не создаёт прав доступа. Профиль и grants проверяются до открытия
каналов, а операции VFS/Styx принимают окончательное решение по правам.
Отмена или отказ после допуска операции не отменяют уже принятые байты: нельзя
автоматически повторять попытку или переключать транспорт, предполагая отсутствие
побочных эффектов.

## Launch handoff / Передача окружения запуска

The launch entry requires an existing live `handoff->group`, a channel-bearing
copied launch description and a valid captured `handoff->flashvm`. It performs no
ticket creation/take, namespace union/reorder, FD transfer, Unibus builtin binding,
module loading, guest startup or init-argument conversion. A console handoff
without display channels refuses this optional attachment without I/O; the host
continues its ordinary console/FD launch independently.

After successful attachment, the caller may free the handoff if it is abandoning
startup: the broker's retained captured fids keep terminal I/O alive independently
of that namespace. If startup continues, the host instead adopts the same handoff
through the existing runtime API. Do not call this constructor twice on the same
record stream or separately consume a UI tag already assigned to its bridge.

A native embedding with an already prepared handoff can use:

```c
unsigned long tags[] = {7, 9}; /* Terminal value frames and an independent resource inbox. */
StyxOrderedOptions options = {0}; /* Native configuration, borrowed until constructor returns. */
options.struct_size = sizeof(options);
options.limit = 4096;
options.window = 1024;
options.lane = 71;
options.tags = tags;
options.count = 2;
StyxOrdered *peer = styxbidirorderedlaunch(client, handoff, &options); /* Owned broker or NULL. */
/* Check peer before use. The queued offer is not proof that negotiation completed. */
/* For an optional display, styxbidirordereduibridge(peer, DVM_UI_DIS, 7) consumes peer on every outcome. */
/* Without that bridge, native clients consume their own inboxes through styxorderedclient(peer, tag). */
```

The FlashVM factory uses the same common attachment before constructing its local
value-channel relays. Failure later in local bridge setup stops the staged broker;
an offer may already have reached the peer. Success of construction is not a
delivery/execution ACK. This is still native launch plumbing, not a complete
network launch protocol or an automatic remote-mode switch in desktop launchers.

## Acceptance / Проверки

A real original-libstyx connection goes through `styxbidirlaunchrequest`, provider
preparation and one-use ticket take. The attachment keeps exchanging independent
application0/7/9 records and selected renewal after both ticket wrappers, original
grant array and the handoff namespace retire. The execution-provider destructor
runs while terminal I/O continues. Disposing the broker leaves the original client
usable for an unrelated walk.

Preflight tests count actual lower constructor admissions: malformed options,
unsafe paths and pre-cancellation open no control. Initial-offer refusal and
cancellation immediately after complete lower setup dispose actual native workers,
then a successful attachment reuses the healthy session. Existing CLI, real local
FlashVM/AS wrapper, C++98 public-header and ordinary control tests exercise the
same constructor. No fake guest execution or graphics implementation is used.
