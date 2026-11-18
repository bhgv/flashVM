# Attached Styx root to launch request / Запрос запуска с подключённым Styx-корнем

[`request.h`](request.h) adds a native adapter from an already attached terminal
VFS session to the existing [launch tickets](../../runtime/LAUNCH-TICKETS.md).
The requester receives status/cancel authority; the provider receives a separate
prepare/take capability. Only the provider selects its permitted execution root.
Preparation builds the private union, and one successful take transfers an owned
handoff for later runtime adoption. Creating, preparing and taking do not run code.

## Connection choice / Выбор соединения

| Purpose | Entry | Transport |
| --- | --- | --- |
| General remote VFS/launch request without FlashVM terminal | `styxlaunchrequest` | Ordinary **or** explicitly configured bidir session. |
| FlashVM terminal connection | `styxbidirlaunchrequest` | **Bidir required**; ordinary clients refuse before root acquisition. |

Для обычных применений доступны оба варианта. Связь DisVM с терминалом FlashVM
обязательно bidir: по ней предполагаются управление, ввод/вывод, клавиши, тапы,
мышь, таймеры, события отрисовки, данные и обращения к переменным через proxy
images — в обе стороны. Это общий терминальный обмен, а не отдельный «канал
нажатий». Для больших или потоковых данных остаётся существующий механизм
дополнительных каналов. Новую схему этих сообщений этот adapter не вводит.

`styxlaunchrequest` accepts ordinary and bidir clients equally, but requires
`spec.flashvm == NULL`. `styxbidirlaunchrequest` also requires that field to be
NULL: it constructs the terminal descriptor itself from the **same captured root**
and explicit receive/send names. This prevents substituting an unrelated ordinary
connection as the terminal grant through this adapter. It requires READ+WRITE+EXEC
as a local ceiling; actual opens and I/O still require permission from the provider.

The client snapshots `PHY_BIDIR_SESSION` when it adopts a carrier.
`phybidirchannel` supplies this local mode; `styxclientbidir` exposes the immutable
snapshot. A bare POSIX fd refuses that capability, and wrappers preserving the
logical session must preserve it. Generic native drivers remain trusted capability
providers, just as for reliable/ordered stream claims. The bit is neither peer
identity, namespace authority nor proof that the reverse server is currently live.
Both peers must already have explicitly configured their bidir adapter and server
owners. The launch helper does not start them or upgrade an ordinary connection.

## Native construction / Создание native-запроса

```c
/* client was already attached on an explicitly configured bidir channel.
 * spec.terminal and spec.flashvm are NULL. Identities, module label and
 * argument bytes were supplied through the host's trusted admission. */
StyxTerminalPaths paths = { "control/in", "control/out", offers };
DVM_LaunchRequest *request = NULL;
DVM_LaunchTicket *ticket = styxbidirlaunchrequest(client, &spec,
    FVM_ACCESS_READ | FVM_ACCESS_WRITE | FVM_ACCESS_EXEC, &paths, &request);
/* client ref is consumed on BOTH success and failure.
 * Check ticket/request; route request only to the authorized provider owner.
 * Requester keeps ticket; provider later uses the ordinary prepare/take APIs. */
```

Receive/send are canonical relative names inside the already granted terminal root;
they may name one duplex node or a pair. The descriptor copies their bytes and
retains the root. Opening those endpoints uses the existing
[terminal channel API](../../runtime/LAUNCH-TERMINAL.md), outside ambient namespace
lookup, so later application union reorder cannot retarget terminal traffic.
The optional `offers` source uses existing per-job additional-channel announcements;
it remains a native capability and is retained by ordinary ticket/handoff rules.
No pointers are serialized by this adapter.

For general requests, use `styxlaunchrequest(client, &spec, access, &request)` with
READ+EXEC and optional WRITE. This entry does not attach a FlashVM terminal even
when the underlying client is bidir. Extra client references must be retained
before either constructor if the caller will continue ordinary RPCs independently.

Both constructors consume one client reference on **every outcome**. `spec` and
path strings are borrowed until return; the provider output is cleared on failure.
The captured root must be a directory and support native cancellation. Preflight
rejects malformed identities, policy, module/argument bounds, access ceiling,
pre-cancellation, pre-filled root/channel grants and invalid terminal paths before
remote acquisition. Native metadata acquisition may wait; in-flight cancellation
uses the existing Tflush path. A file attach, failed stat, malformed response or
link failure cannot publish either launch capability.

The adapter copies description/arguments through the existing ticket constructor
and exposes neither capability until its final cancellation check. It does not
select an error vocabulary: an Inferno lookup profile must be chosen beforehand
by the native host when appropriate. Ordinary opaque-error behavior is preserved.

## Prepare, take and lifetime / Подготовка, take и lifetime

1. Requester observes/cancels its ticket; it has no provider root or take authority.
2. Provider reads the copied description/arguments and accepts or refuses.
3. `dvm_launch_request_prepare` checks expected provider/session associations and
   builds the normal private terminal/provider union from the remote terminal root.
4. `dvm_launch_request_take` transfers the environment once. Native concurrent
   contenders have one winner; a taken environment cannot be revoked through the ticket.
5. After adoption/retirement, release remaining handoff owners normally. Ticket,
   request, journal, namespace, open channels and physical carrier have distinct lifetimes.

A handoff retains remote root/channel grants independently of requester/provider
wrappers. The host must retain the physical/bidir link while any protocol user
still needs it. Explicit transport shutdown remains a real failure; it does not
reconnect, migrate, retarget or replay work. `phybidirfree` still aborts the link.

These are native capability adapters, not a network launch RPC protocol. The host
must authenticate/authorize routing and bound concurrent root acquisitions before
calling them. They do not reserve slots in the existing launch-service queue;
no unbounded queue admission or resource reservation is implied. Network request
framing, remote module startup, typed init parameter conversion, proxy-variable
semantics and full FlashVM terminal dispatch remain separate integration work.

## Verified / Проверено

Actual original libstyx clients/servers on one raw PTY cover ordinary requests and
bidir requests with simultaneous opposite sessions. Tests check copied descriptions
and binary arguments, malformed/pre-cancelled refusal, provider identity rejection,
pending/ready cancellation, provider refusal, a one-winner native take race,
independent journals and live remote reads after both wrappers retire.

FlashVM-terminal tests require bidir, reject ordinary clients, and carry opaque
input/output bytes through the pinned remote descriptor after application union
reordering. This is native channel plumbing, **not execution of a FlashVM movie or
implementation of keyboard/timer/proxy schemas**. Existing actual-player tests
continue independently; no graphics primitives move into DisVM.

An independent raw PTY peer injects file attaches, first/second metadata failures,
wrong response tags, transport loss and cancellation during both metadata stages.
Cancellation emits one ordinary Tflush, leaves no published ticket/request, and
preserves the caller's independently retained healthy session where applicable.

All 328 Linux64 native SDK tests pass normally and under ASan/UBSan, with
LeakSanitizer disabled. Imported Inferno sources and FlashVM graphics code are unchanged.

The [generic bidir terminal record transport](../terminal/README.md) now opens
this job's control endpoints, runs independent native RX/TX owners and delivers
one verified S-record at a time. Concrete event/proxy dispatch and automatic
IDE host attachment remain separate from this transport layer.

The [shared ordered attachment](../terminal/order/broker/ATTACHMENT.md) can now
consume the **borrowed channel description** of the actual one-use handoff through
`styxbidirorderedlaunch`. The handoff itself remains owned and unchanged; this
constructs the terminal broker only, without starting guest code or changing init.
The CLI and optional FlashVM local-channel bridge share this native setup path.
