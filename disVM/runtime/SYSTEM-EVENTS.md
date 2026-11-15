# Automatic native system inboxes — runtime 0.131

Copyright (c) 2026 bhgv

## Delivery / Доставка

Every resident owns a separate native inbox, even when residents share a namespace.
The [source journal](vfs/changes.c) pushes accepted reports through the
[native notice hub](vfs/notice.h) directly into subscribed inboxes. Namespace
mutations publish after releasing the namespace mutex. Their native callbacks
update subscriptions before the mutation returns; no Limbo registration is needed.

Каждый resident получает отдельную native очередь, даже при общем namespace.
Источник непосредственно доставляет уведомление через native push. Подписки
обновляются автоматически при bind/mount/unmount и замене namespace через pctl.
Ни исполнение получателя, ни чтение устройства не запускает сбор событий.

There is no scheduler-boundary polling, cooperative handler, guest callback,
helper sleep or recipient pause. Native callbacks enqueue copied scalar/path
messages; they never execute the recipient or perform provider I/O. Short native
mutex sections protect subscription lifetime and the bounded queue. They do not
require an execution stop or an acknowledgement from the receiving resident.

Нет cooperative обработчиков, опроса на границах исполнения, sleep helper или
приостановки получателя. Callback источника только ставит native сообщение в
очередь. Уведомления не требуют guest ack и не смешиваются с запросами управления
процессами: используется отдельный endpoint существующего owner_messages engine.

## VFS view / Представление

Boot installs a contextual readonly provider at `/dev/sysevents` using the normal
namespace binding mechanism. Opening it captures the calling native owner's inbox;
there is no caller-supplied PID and no global guest-accessible provider registry.
Binding, hiding, unmounting and NEWNS apply normally. An explicitly inherited open
FD retains its original inbox capability, as an ordinary open descriptor does.

`/dev/sysevents` — обычный readonly узел VFS. Его можно скрыть или перепривязать;
специального обхода namespace по строке пути нет. Открытие выбирает очередь своего
потока. Унаследованный FD сохраняет исходную capability. В сообщениях используются
только виртуальные точки монтирования текущего namespace, без host source paths
и provider-local resource addresses. Уведомление не предоставляет права на файл.

| Operation / Операция | Current behavior / Текущее поведение |
| --- | --- |
| `Sys.read` | Exactly one whole queued record / Ровно одна целая запись |
| `Sys.readn` | Stops after that record through native DVM_IO_RECORD / Останавливается после одной записи |
| Short buffer / Маленький буфер | Error, shared inbox head preserved across FD close/reopen / Ошибка, запись остаётся в общей очереди |
| Empty live inbox / Пустая живая очередь | Temporary-unavailability error, not EOF or a fake event / Временная недоступность, не EOF и не фиктивное событие |
| Write / Запись | Refused; no guest registration/ack protocol / Отказ, регистрация и ack не нужны |
| Owner exit / Завершение owner | Queued records and exit notification remain drainable, then EOF / Остаток и exit доступны, затем EOF |

Multiple available events are never concatenated by one read, including readn.
Native frames carry the record boundary independently from EOF. A read does not
refresh provider metadata or register a watch. A caller may separately reread
state at the provider after receiving a change notification.

Несколько накопленных событий возвращаются отдельными чтениями. Перечитывание
состояния провайдера после уведомления — отдельная операция; очередь не исполняет
его за Limbo. Пустая очередь не останавливает поток в ожидании события.

## Scope, loss and lifetime / Scope, потери и lifetime

The native inbox distinguishes provider change, namespace revision, resync and
owner exit. Provider classes remain RIGHTS/NAMESPACE/CONTENT/CONNECTION/LOAD/NETWORK.
Source session/generation fences reject obsolete reports. Reordered deliveries
with a generation gap broaden the invalidation to all classes, preserving safety.
Changing scope purges old queued messages, including the inbox-owned head.

Очередь ограничена32 сообщениями. Overflow или allocation failure дают явный
resync вместо неограниченного роста или ложного EOF. При resync следует перечитать
текущий scope; доставка каждого промежуточного изменения не гарантируется.
При смене namespace старые сообщения и общий head удаляются под inbox mutex.

[Inbox lifecycle](vfs/sysevents.h) retains native namespace/provider state, never
Prog or managed pointers. Owner retirement detaches source and namespace taps;
retained FDs can outlive the owner. In-flight callbacks retain their own context,
so detach neither waits for nor stops a recipient. Source callbacks run outside
journal/hub mutexes; final destructors also run outside those mutexes.

## Atomic consumption and final notices / Atomic consumption и final notices

Runtime0.131 keeps the next unread node in the inbox, not inside an opened FD.
An undersized read copies no bytes and consumes no notification. Closing that FD
cannot discard the head or let another reader overtake it. A later sufficiently
large read through any FD for the same inbox receives the whole head. Zero-byte
reads leave it untouched. Independent FDs compete for one FIFO; separate residents
still have independent inboxes. Native copy and consumption share one inbox mutex.

Маленький буфер не переносит запись в FD: она остаётся общей для очереди. Close,
reopen и другой reader не теряют и не переставляют её. После успешного копирования
всех bytes запись удаляется ровно один раз. Один FD по-прежнему сериализуется
обычным file gate; два разных FD могут читать одну очередь конкурентно.

Overflow purges stale detail and sets one scalar resync flag, retaining the report
that triggered overflow when its allocation succeeds. Repeated loss before resync
consumption coalesces to that flag. There is no additional queued duplicate resync.
An allocation refusal also sets this flag, so delivery of the loss notice itself
needs no allocation. An undersized read cannot clear the flag.

Overflow и OOM дают один allocation-free resync. Сам report, вызвавший overflow,
сохраняется при успешной allocation. Повторные потери до чтения не размножают resync.
Обычных message nodes максимум32, плюс два scalar flags: resync и final exit.
Held inbox head входит в эти32, а не образует дополнительную очередь на каждый FD.

Retirement sets an allocation-free final-exit flag exactly once and detaches native
subscriptions. Remaining resync and FIFO records are read one at a time; exit comes
last, then EOF through every retained FD. Full queues, allocator failure, repeated
retirement and short exit reads cannot erase or duplicate the final notification.

Retirement не выделяет память для exit и не вытесняет обычные записи ради него.
Даже при заполненной очереди и отказе allocator сначала выдаются resync/остаток,
потом один exit и затем EOF. Повторный retire не добавляет ещё один exit.

## Experimental format and limits / Экспериментальный формат и границы

**The external message format is not agreed or frozen.** The current textual FD
adapter exists for executable validation only. Event names, fields, encoding and
future transport must be agreed separately; do not treat fixture parsing as a
stable guest ABI. No Sys.m signature, Dis instruction or upstream snapshot changes.

**Окончательный формат ещё не согласован.** Текущий текстовый adapter — временная
проверяемая реализация. Сейчас закреплены способ доставки и одно событие на чтение,
а не постоянный внешний формат. Это расширение среды, не новый Inferno/Styx message.

Automatic subscriptions cover journal-reporting native providers. The source must
publish an actual native change report. There is no implemented Styx receiver,
wire heartbeat, external host-filesystem watcher, metrics sampler, balancing or
automatic migration of an open file between union layers in this checkpoint.
Mounting a host directory does not turn it into an OS filesystem notification source.

Автоматическое подключение получателей уже есть; обнаружение изменений самим
источником остаётся задачей соответствующего native provider/transport. Это не
готовый сетевой heartbeat или наблюдатель внешних изменений host filesystem.

## Acceptance / Проверка

[Native inbox tests](vfs/test_sysevents.c) prove source-to-inbox delivery before
any owner binding or device open, independent inboxes, namespace isolation,
overflow, unmount and retained FD retirement, without source stat/open callbacks.
[Notice lifetime tests](vfs/test_notice.c) exercise self-detach and two concurrent
native publishers while foreign subscriptions are repeatedly attached/detached.

[Real guest fixture](fixtures/provider-watch/main.b) checks ordinary namespace
hiding and readn boundaries with multiple events, using the production boot device.
The [private host source](test_provider_watch.c) supplies native reports; it does
not inject a replacement system device or scheduler polling hook.

[Atomic queue tests](vfs/test_sysevents_queue.c) inject real calloc failures and
verify cross-FD head retention, untouched short/zero-read destinations, one resync,
full-queue retirement without allocation, exactly one exit and two competing native
readers consuming every sequence once. [Source/namespace race](vfs/test_sysevents_race.c)
publishes2000 real journal reports while rebinding the source200 times and switching
another inbox between visible/hidden namespaces. No guest executes to service those
subscriptions; source stat/open callbacks are forbidden and final cleanup is checked.
