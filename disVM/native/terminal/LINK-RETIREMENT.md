# Connection retirement / Завершение соединения

The control transport now subscribes directly to its exact native Styx client's
terminal source. Previously it learned about failure only from its RX/TX RPC
results. A full RX queue can park the receiver outside any RPC while the writer
is idle. Closing the underlying client in that state did not necessarily wake
the control, its route or native consumers of empty route queues.

RU: Обрыв соединения замечается независимо от RX/TX. Если RX ждёт места в полной
очереди, а TX простаивает, native-подписка всё равно завершает transport и будит
его потребителей. Гостевые потоки для этого не опрашиваются и не приостанавливаются.

## Outcomes / Результаты

| Event | Result |
| --- | --- |
| Client terminal notification, no earlier outcome | Latch `STYX_CONTROL_READ_FAILED`; wake RX/TX capacity/completion waits and retirement subscribers. |
| Earlier explicit local stop | Preserve `STYX_CONTROL_STOPPED`; later client loss does not replace it. |
| Earlier protocol, read or write outcome | Preserve the first reason. A simultaneous connection notification may win before a worker reports its more specific result. |
| Already accepted RX record | Remains readable, exactly one record per call, then END. |
| Verified record blocked before RX admission | Not admitted after retirement; freeing capacity does not revive the transport. |
| Full route mailbox | Existing route-retirement subscription wakes its dispatcher and empty-mailbox consumers independently of queue capacity. |

The notification means that this connection has retired. A heartbeat policy
timeout, explicit client shutdown or carrier failure does not prove that the
remote machine has died. There is no automatic reconnect, replay, alternate
provider lookup, new wire message or new numeric S-tag.

Local control/route stop still leaves the shared client with its host. The new
subscription observes the client; it does not close it. The physical carrier
remains behind RemotePhy. No TCP/UDP-specific behavior is added to the adapter.

## Setup and ownership / Создание и владение

The constructor completes its captured-root and endpoint setup before subscribing,
then starts the two native workers. All callback-visible state is initialized
before registration because a terminal source may replay immediately. A failed
subscription or an already observed terminal outcome refuses construction and
rolls back private fids/workers. It never publishes weaker failure monitoring.

The subscription owns an independent context with a short native mutex and a
borrowed control pointer. The callback latches the first outcome, requests active
native RPC cancellation and signals conditions/subscribers. It performs no wire
I/O, joins, guest dispatch or target wait. Existing retirement-callback restrictions
still apply: subscribers must not free their owner or join its workers.

After external users retire, disposal stops and joins the owned workers, detaches
the context's control pointer under its mutex, then unsubscribes without waiting
for future callbacks. An already admitted callback may finish later using only
its independently retained context. It cannot access the freed control. The
last subscription/callback reference disposes that context.

RU: Подписка получает собственный context; указатель на control отделяется до его
освобождения. Поздний callback безопасно обнаруживает отсутствие владельца.
Сначала завершаются внешние пользователи, затем свои native workers; общий client
не закрывается при локальном stop/free.

## Verification / Проверка

The [12 native scenarios](tests/link/cases.cmake) use original libstyx server
walk/open/fid handling and real S-record framing over explicit bidir pumps on an
isolated local carrier. A native checkpoint observes RX actually waiting for
capacity with no active read RPC. The tests then close the original client or
break the actual carrier, leaving queues untouched until retirement arrives.

They exercise raw RX and general/UI/typed route backpressure, automatic resource
consumer/server retirement, retained record heads, watch-allocation refusal and
retry, sticky terminal replay during registration, first-stop preservation and
callbacks deliberately delivered after control free. The fixture's socketpair is
only its local carrier; production connection behavior stays carrier-independent.

Negative controls replace the constructor body with the preceding implementation
in the same new test build. Both explicit client shutdown and actual carrier
break then fail the bounded full-RX retirement check. Normal and ASan/UBSan
builds pass all 652 cases, including these 12 new scenarios.

## Dynamic recipients / Динамические адресаты

Recipient mailboxes now [grow without blocking the dispatcher](route/DYNAMIC.md).
The raw native transport-ring tests remain separate; route scenarios now observe
six committed recipient messages rather than an application-capacity wait.
Connection retirement, first reasons and detached callback lifetime are unchanged.
The original 652-case result above records the earlier fixed-mailbox stage.
