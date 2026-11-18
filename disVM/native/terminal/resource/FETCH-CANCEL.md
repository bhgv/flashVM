# Client fetch cancellation / Отмена загрузки у клиента

Unibus resource clients now propagate native cancellation through the entire
client-side operation: initial offer readiness, ordinary resource-fid RPCs and
scope-aware native staging I/O. Queue wakeups alone could not interrupt a caller
already waiting in a resource `Tread` or inside its native sink.

RU: Отмена доходит до ожидания первого анонса, RPC отдельного resource fid и
native I/O внутри sink. Используются существующие scope, свежие Styx-токены и
`Tflush`. Гостевые потоки не опрашиваются и не приостанавливаются.

## Sources and isolation / Источники и изоляция

| Source | Effect |
| --- | --- |
| One-shot client's route retires | Request only that handle's native cancellation source. Its active sink/queue waits wake. |
| Announced receiver stops, closes or fails | Request its lifetime source; active fetch-local sources receive cancellation. |
| Caller cancels one announced fetch | Request that fetch's independent child source; receiver and other callers remain usable. |
| Caller was already cancelled | Sticky replay refuses readiness/RPC admission under the fetch's source. |
| READY changes generation | Existing transfer may finish, but final generation fence rejects stale origin/data admission. It does not retire the receiver's lifetime source. |

Every announced fetch creates its own native cancellation source and forwards
receiver lifetime and optional outer-caller cancellation into it. There is **no
reverse forwarding** into either parent. A cancelled caller can coexist with
another successful ordinary per-fid fetch on the same receiver/client. The caller's
original scope is restored before return.

The public readiness snapshot/wait APIs keep their existing meaning. Fetches use
an internal cancelable readiness wait: cancellation signals its native condition
without consuming offers or changing shared availability. Its private callback
context detaches receiver access before nonwaiting unsubscribe, including races
with an already-admitted callback. It does not retain a pointer to a caller stack.

## Ordinary resource fids / Обычные resource fids

The ordinary movie loader now reuses the existing VFS scope-to-Styx cancellation
binding. Each logical `Twalk`, `Topen`, data RPC and consuming `Tclunk` gets its own
fresh single-use native token under the current scope. Source cancellation wakes
that admitted native operation through the existing Tflush engine. Late callbacks
retain the old token and cannot target a later RPC.

Confirmed short prefixes retain their original offsets and acknowledgement rules.
Original replies still win the native reply/flush race. Failed or uncertain effects
are not replayed. The higher-level fetch applies its own final scope and generation
checks before publishing success. Cleanup consumes the owned fid even when scope
setup fails or cancellation is already requested; the existing detached best-effort
cleanup path handles unacknowledged clunks.

No second 9P connection, transport reader, listener, protocol opcode or capability
lookup is introduced. The exact granted client remains associated with its source. Typed-announced
fetches walk from the route's [retained grant root](../GRANTS.md) under the
fetch-local scope; setup clones, endpoint opens and consuming cleanup also use
the existing scope-to-Styx cancellation bridge.
Legacy noncancellable clients retain their ordinary RPC deadline behavior; this
change does not promise to interrupt their blocking implementation. Native Unibus
routes already require the explicit native client capabilities used here.

## Staging and origin / Staging и origin

A sink stays on the original native fetch owner and sees the current native scope.
The client checks cancellation around each synchronous sink invocation. A sink can
subscribe to that scope to wake its own native I/O, as a provider can; arbitrary
sink code is not forcibly preempted. Existing native I/O bounds still apply. No
new total-file timer or watcher thread is added.

Received bytes remain caller-owned staging. A failed fetch does not undo a sink's
external effects: commit/discard policy still belongs to the caller. No refused
sink is replayed. A sink's own non-success return remains its refusal at the reply
parser boundary; enclosing lifetime/generation failure still refuses the fetch.

Both raw and typed-announced loaders keep the resolved origin in private storage
through END, cleanup and their final admission checks. A generation change after
staging cannot leave an updated caller origin alongside a failed return. Argument,
subscription, cancellation and protocol failures leave the caller origin unchanged.

RU: Sink выполняется на исходном native owner и получает scope. Его собственный
I/O может подписаться на отмену; произвольный callback принудительно не прерывается.
Bytes остаются staging caller. Origin публикуется только после финальных проверок,
включая смену поколения после передачи bytes. Отказ не повторяет sink/RPC.

## Lifetime and failures / Владение и отказы

Route/lifetime forwarding callbacks retain only independent native sources. They
store no client, receiver, sink, provider or guest pointers. After unsubscribe,
an already-admitted callback can safely finish using its own source pin. Local
fetch scope rollback removes every adopted subscription and restores any entered
scope. Constructor subscription refusal leaves the host's route/client owned and
usable; it never publishes an object with silently weaker cancellation.

As before, hosts must retire all external fetch/status/stop users before freeing
a handle or receiver. A receiver free joins only its offer worker, not arbitrary
external fetch callers. Cancellation makes that shutdown easier to coordinate;
it does not authorize concurrent use of freed objects. One-shot handles still
admit exactly one valid attempt and cannot be reused after an incomplete exchange.

## Verification / Проверка

The [original 9P fixtures](../tests/fetch_cancel/announced.c) hold an actual original
libstyx resource `Tread` pending. Its server acknowledges admission on that same
requesting client's control stream with a **fixture-only S-record**. Stop, CLOSED,
caller cancellation or a real uncertain TX then causes the resource RPC's ordinary
Tflush/cleanup. The test checks the original deferred-read release, prompt completion
before the normal RPC timeout, preserved origin and subsequent ordinary client use.
The receipt is test synchronization, not a new production tag/schema.

Other cases block a native sink on its current cancellation source, overlap a
successful independent per-fid fetch, recover after caller cancellation, cancel
initial readiness and inject constructor/first-parent/second-parent/readiness
subscription failures. A raw-session case changes generation during staging and
checks that failure preserves the origin sentinel. Typed one-shot sink tests cover
route/local/caller cancellation and constructor rollback. Binary fixture bytes are
not executed SWF; actual channel/loader execution keeps its separate test scope.

Negative controls verify that the previous announced callback waits for RPC timeout
and that premature raw-origin publication fails the generation/sentinel test.
