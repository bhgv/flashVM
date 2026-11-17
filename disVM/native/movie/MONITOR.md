# Autonomous terminal control reader / Автономный reader терминала

The native movie SDK now owns a **background control reader** as well as the
[outgoing writer](OUTBOX.md). It receives announcements while the actual FlashVM
player runs on its own thread, updates the shared scoped receiver, and signals a
native readiness descriptor for the host. Application code no longer pumps the
control stream between movie loads. Neither the worker nor its callbacks enter
Dis/AS heaps, SDL, or guest scheduling.

Native SDK теперь сам читает управляющий поток в отдельном I/O thread. Поколение
провайдера, закрытие и ошибки обновляются независимо от выполнения SWF. Хост
получает актуальное состояние через native fd готовности. Гостевые потоки не
останавливаются и не участвуют в доставке сообщений.

## Host API / API хоста

| API | Contract / Контракт |
| --- | --- |
| [`styxmoviecontrolstart(session, relative, input)`](monitor.h) | Walk/open one read-only control file on the session's exact granted connection, claim exclusive input and start an owned reader; NULL on failure. / Открыть control file на connection сессии и запустить reader. |
| `styxmoviecontrolfd(monitor)` | Borrow readiness fd; do not read/close it directly. / Borrowed fd готовности. |
| `styxmoviecontrolstate(monitor, &state)` | Copy latest state and acknowledge native readiness atomically; one state consumer. / Snapshot и подтверждение readiness под общим lock. |
| `styxmoviecontrolstop(monitor)` | Idempotent nonwaiting request: retire receiver and cancel the current native read. / Завершить receiver и отменить текущий read. |
| `styxmoviecontrolfree(monitor)` | Stop and join only the owned I/O worker;1 consumed,0 leaves ownership after self/join failure. / Join собственного I/O worker, без ожидания гостя. |
| [`styxmoviesessionretain/free`](receiver.h) | Explicit session owners; the last owner detaches its terminal watch. / Явные owners сессии. |

Setup performs native Twalk/Topen and may block. It requires an already authorized,
watchable native fd client with **at least two RPC slots**. One pending watch read
uses a slot; resource fetches, other RPC and heartbeat continue on the same native
connection. Legacy serialized transports and insufficient quotas refuse before
control I/O. The monitor does not dial, authenticate, install paths, or grant access
from an announcement name. All ordinary VFS/9P access rules remain in force.

The constructor retains the session independently. The caller may drop its own
session owner while the monitor keeps it alive; fetch users still need a live
owner/pin through use. The monitor permits no second automatic reader and rejects
manual `styxmoviecontrol` feed without mutating receiver state. A partial manual
record also prevents claiming a fresh stream. Failed setup releases its claim and
temporary owners; input context remains caller-owned. Actual transport errors
keep the existing client's terminal-state behavior.

```c
StyxMovieSession *session = styxmoviesession(granted_client);
StyxMovieMonitor *monitor = styxmoviecontrolstart(session, "ctl", &native_dispatch);
/* Existing native event loop waits for styxmoviecontrolfd(monitor). */
StyxMovieMonitorState state;
styxmoviecontrolstate(monitor, &state);
/* When ready, run the player on its ordinary owner thread: */
loader.fetch = styxmovieannouncedfetch;
loader.userdata = session;
```

The host checks constructor/status results, keeps owners through borrowed use,
and retires external state/fetch callers before final free. On normal teardown,
`styxmoviecontrolfree` joins the I/O worker, then releases its native dispatcher
context and session pin. It does not shut down the borrowed client. Cancelling an
empty read uses native Tflush fencing; fid cleanup uses the existing clunk-later
mechanism. Flush/cleanup failures retain that mechanism's conservative link policy.
There is no `pthread_cancel`, guest safe-point request or global executor stop.

## Generic control dispatch / Общий управляющий поток

The optional `StyxMovieInput` context contains a native `receive(state, bytes,
size)` callback and final `release(state)`. Use NULL only for an explicitly
dedicated announcement-only direction. For a shared terminal direction, provide
the host's **generic native dispatcher**, rather than a per-application loader.
It receives the original raw fragments, including tag0 bindings and unrelated
bound message types. The movie receiver does not steal those messages.

Raw fragments can end anywhere, including within CRC/integer/body fields. The
dispatcher copies them into bounded native queues or its existing parser and
validates complete records before acting on payloads. It returns1 accepted,
0 refusal; refusal retires the movie receiver before admitting that chunk there.
The callback runs only on the I/O worker, without monitor/source locks. It must
not enter a VM/SDL, block, retain borrowed bytes, or reenter the monitor. The
context is adopted only after successful thread start and released after join;
callbacks can begin before the constructor returns its successful result.

Для общего control stream хост задаёт один общий native-диспетчер. Он получает
исходные фрагменты, а не только movie-события; остальные типы записей не теряются.
Перед использованием payload диспетчер сам проверяет полный record. Передача в
Dis/AS/SDL делается через обычные bounded native queues и owner-thread обработку,
а не прямым вызовом гостя из reader. Лимит отдельной control-записи остаётся
`MOVIE_FRAME` —8320 bytes. Для больших media/bulk данных используются отдельные
потоки/каналы; этот reader не является неограниченным media-demux.

## State and lifecycle / Состояние и время жизни

`StyxMovieMonitorState` is a copied **latest observation**, not an event queue or
an authority grant. Intermediate observations may coalesce. Its generation and
available fields follow the receiver:1 ready,0 pending,-1 retired. The reason is:

| Reason | Meaning / Значение |
| --- | --- |
| READING | Native worker live; available may already be-1 during final cleanup. / Worker ещё завершает работу. |
| CLOSED | Receiver retired by CLOSED or observed terminal link state. / Закрытие receiver. |
| EOF | Unexpected ordinary control EOF; receiver retired. / Неожиданный EOF. |
| FAILED | Transport/framing/native-dispatch failure; receiver retired. / Ошибка связи, framing или dispatcher. |
| STOPPED | Host requested stop while live; receiver retired. / Запрошена остановка I/O reader. |

A source process is not stopped by any of these reasons. Already executing SWF
also remains under its ordinary player owner; this API does not suspend or kill
it. Subsequent fetches refuse once the receiver retires. A changed generation
observed before acquisition completion discards staged bytes as before.

Readiness acknowledgement and signalling share one short native mutex, closing
the drain/next-wakeup race. Unregister readiness after a terminal reason. Each
Tread gets a **fresh one-operation cancellation token**; the current token is
published under its lifetime lock, so stop cannot target a later read or race its
free. Stop between reads prevents the next admission. Calling stop is nonwaiting;
final free may wait for the native read's flush grace. Never call final free from
receive/release callbacks. Manual `styxmoviecontrolend` alone is not a substitute
for cancelling an automatic reader: use this monitor's stop/free API.

Настройки heartbeat принадлежат существующему соединению и не меняются reader.
`readwatch` отключает обычный idle deadline для долгого control Tread, сохраняя
heartbeat, terminal observation и cancel/flush. После stop сессия закрыта: для
нового reader создаётся новая сессия с заново выбранным разрешённым control path,
без переноса недочитанных фрагментов.

## Verified scope / Проверки

[`tests/wire.py`](tests/wire.py) runs56 real TCP/FlashVM interpreter/JIT scenarios,
including22 new autonomous-reader cases. Monitor control responses are fragmented
to at most3 bytes per Tread. Acceptance covers:

- actual player execution while an independent watch read waits;
- source generation update during acquisition, followed by a successful fresh load;
- receiver kept alive after its original caller drops ownership;
- externally gated stop after actual server admission of a pending read;
- normal cancellation preserving the connection and source authority;
- unrelated bound S-record delivery to the native dispatcher;
- dispatcher refusal, corrupt CRC, control EOF and native disconnect with a valid
  local SWF present, all refusing fallback;
- single-slot rejection, duplicate reader/manual-feed refusal and partial-input refusal;
- owner-local injected pthread_create failure preserving session/input ownership;
-24 immediate start/stop/drop cycles per execution profile, including caller-first free.

The existing framing/admission/outbox suites and genuine deferred-RPC/Rflush
acceptance remain enabled. Normal and ASan/UBSan builds pass; LeakSanitizer is
disabled. Native components use existing Linux64 SDK inputs and original
lib9/libstyx. Runtime/FlashVM versions and Limbo interface layouts are unchanged.
A full authenticated distributed launcher, automatic external FlashVM process
creation and generic media/input dispatch remain further integration work.
