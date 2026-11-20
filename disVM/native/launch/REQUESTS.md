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

## Submit to the provider queue / Подача в очередь провайдера

`styxbidirlaunchsubmit` captures the bidir root and submits the resulting spec
through a previously authorized [submit endpoint](../../runtime/LAUNCH-SERVICE.md).
It returns the requester ticket. The service owner dequeues the separate review
and then prepares and takes the handoff. Queue admission and guest startup remain
separate observations. The guest fixture uses this route before importing the
private VFS and exchanging the startup receipt through its granted Unibus files.

`styxbidirlaunchsubmit` захватывает bidir-корень и подаёт описание через заранее
выданный endpoint очереди. Вызывающий получает ticket, а владелец очереди
забирает review, готовит окружение и получает handoff. Успех подачи сообщает о
приёме в очередь. Старт гостя подтверждается отдельно через выданные файлы Unibus.

### Dispatch an existing S-record / Передать существующую S-запись

The authenticated host dispatcher can pass a decoded record to
`styxbidirlaunchrecord`. The host binds the expected nonzero tag to an
argument-free spec for one permitted application. The record body supplies up to
65536 bytes of startup arguments, including NULs. Foreign tags and oversized
bodies are refused before Styx I/O. A successful submission copies the arguments
and returns a ticket. The guest reads its own copy through `Launchcontext`.

Диспетчер с уже проверенной сессией передаёт декодированную запись в
`styxbidirlaunchrecord`. Host связывает ненулевой тег с описанием разрешённого
приложения без аргументов. Тело записи передаёт до 65536 байт аргументов, включая
NUL. Чужой тег и превышение размера отклоняются до Styx I/O. При подаче аргументы
копируются, а гость читает свою копию через `Launchcontext`.

The dispatcher owns authentication and assignment of the tag. The adapter
consumes one client reference per call. It does not read from the connection
directly or define a numeric wire tag.

Диспетчер отвечает за проверку сессии и назначение тега. Адаптер поглощает одну
ссылку на клиент за вызов. Он получает готовую запись, а номер тега задаёт host.

### Complete encoded frame / Полный закодированный кадр

[`styxterminallaunchframe`](../terminal/launch_frame.h) accepts one complete S-record
from the host's existing reader. The common codec verifies checksum and framing.
A truncated record or any trailing bytes cause refusal before queue submission.
This entry is available to hosts that already own a complete encoded frame.

[`styxterminallaunchframe`](../terminal/launch_frame.h) принимает одну полную
S-запись от существующего reader. Общий кодек проверяет длину и контрольную сумму.
Обрезанная запись или лишние байты отклоняются до подачи в очередь. Этот вход
доступен, когда host уже получил полный закодированный кадр.

### Typed terminal mailbox / Типизированная очередь терминала

[`styxbidirroutelaunchnext`](../terminal/route/launch.h) connects an existing
dispatcher to provider admission. Register the launch schema in
`styxbidirbindingroutes` and dedicate its typed queue to this consumer.
The host supplies the authorized application spec and negotiated tag. The
adapter takes the terminal root from the same client as the dispatcher.
A small buffer leaves the head queued. A consumed but refused request returns
a distinct status. Metadata acquisition runs on the caller after dequeuing,
so the dispatcher can continue receiving other messages.

[`styxbidirroutelaunchnext`](../terminal/route/launch.h) соединяет диспетчер с
очередью провайдера. Зарегистрируйте схему запуска в `styxbidirbindingroutes`
и выделите её очередь этому потребителю. Host задаёт разрешённое приложение и
согласованный тег. Корень терминала берётся из соединения самого диспетчера.
При недостатке буфера запись остаётся в очереди. Отказ после получения записи
возвращается отдельным статусом. Метаданные читает вызывающий после получения
записи, поэтому диспетчер продолжает принимать остальные сообщения.

The guest fixture serves the binding and request through a separate read-only
Styx node. The actual dispatcher classifies the request, the provider adopts
its queued handoff, and the guest confirms startup on its Unibus record channel.
The guest's filtered union hides both raw terminal nodes.

В сквозной проверке отдельный узел Styx отдаёт объявление привязки и запрос.
Диспетчер выбирает очередь, провайдер принимает окружение из этой очереди,
а гость подтверждает запуск по каналу Unibus. Оба исходных узла терминала
скрыты из union гостя.

### Send from a selected VFS name / Подача по выбранному имени VFS

A resolver installed with [`styxmountproviderlaunch`](../vfs/mount.h) can return
a [transport address](../terminal/launch_send.h) backed by an exact writable
Styx request file. Resolve the visible application alias with
`dvm_launch_route_resolve`, then use `dvm_launch_address_send` to send copied
startup bytes. The resolver binds the application to the request file and tag.
The peer supplies neither a host pathname nor a local endpoint pointer.

The request file service must accept exactly one complete S-record in an
offset-zero write and acknowledge it only after it retains the request.
It may hand complete encoded records to `styxterminallaunchframe` or an
existing typed dispatcher. One complete confirmed write returns DELIVERED.
A partial or uncertain write returns UNKNOWN and is never automatically
retried. Delivery and the guest's startup receipt are separate observations.

Resolver, установленный через `styxmountproviderlaunch`, может вернуть адрес
с правом записи в точный узел Styx. `dvm_launch_route_resolve` выбирает приложение
по видимому алиасу. `dvm_launch_address_send` передаёт стартовые байты по
выданному адресу. Служба принимает одну полную S-запись с нулевого смещения
и подтверждает её после сохранения запроса. Частичный или неизвестный исход
не вызывает повторной отправки. Подтверждение доставки и старт гостя различаются.

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
