# Automatic native system inboxes — runtime 0.132

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

Device construction publishes one fully initialized factory with an atomic
compare/exchange. Concurrent constructors return one owned winner; unpublished
losers release their private state without clearing the winner's identity.
Allocation failure leaves no reserved singleton. Bound roots and opened handles
pin the factory, so recreation is refused until the last such reference retires.
The global identity is compared only, never dereferenced without a caller-owned
reference. It does not select a recipient or grant access.

Конкурирующие конструкторы публикуют ровно одно устройство. Проигравшие освобождают
свои объекты, не сбрасывая регистрацию победителя. Отказ памяти не резервирует
устройство. Пока его удерживает bind или открытый FD, повторное создание отклоняется;
после последнего освобождения оно снова возможно. Глобальная идентичность не даёт
прав доступа и не заменяет выбор очереди текущего потока.

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

## Binding reconciliation / Согласование подписок с namespace

Runtime0.132 retains the exact native watch and notice tap for an unchanged binding
within the same namespace group. Matching requires layer identity, provider factory
and virtual mount point; a replacement at the same path has a new binding identity
and receives a fresh subscription. Removing a binding detaches only that watch.
Cwd/import-directory changes and unrelated bindings do not reset source epochs or
watermarks, allocate replacement watches or repeat initial provider snapshots.

Неизменившиеся привязки сохраняют native context/tap и source session/generation.
Изменение namespace всё равно выдаёт отдельное namespace notification и очищает
сообщения прошлого scope. Это уведомление означает пересмотр assumptions текущего
namespace; оно не изображается как повторный initial event каждого провайдера.
Для реально новой binding по-прежнему выполняется initial journal snapshot.

Reconciliation continues after failure to allocate a new watch or tap. Existing
valid subscriptions remain live, removed bindings are detached, and explicit resync
marks the incomplete set. A later native namespace mutation retries missing watches;
reading the event device neither registers nor repairs subscriptions. There is no
background allocation-retry worker in this checkpoint, so an idle incomplete set
waits for that later native mutation. No guest polling/registration is required.

Отказ новой watch/tap не отключает остальные источники. Partial native contexts
освобождаются после выхода из inbox/group locks; последующее native изменение
namespace повторяет создание отсутствующих подписок. Пока такого изменения нет,
неполнота явно отмечена resync, а работающие подписки продолжают push. Чтение из
Limbo не является точкой ремонта или cooperative обработчиком.

Namespace replacement through pctl deliberately discards the previous group's
watch set; group-local identities are never reused across unrelated groups.
A source reconnect remains a new source session even on a reused watch. Stale
reports from the old session cannot replace current state.

When the scope counter is exhausted, the callback detaches every old source watch,
rejects already admitted old callbacks and sets resync. It never wraps the epoch,
keeps a stale binding visible, pauses a receiver, or invents owner exit/EOF. The
owner remains live; further pctl replacement is refused by the existing epoch check.
Actual retirement still supplies the ordinary final exit notification.

При исчерпании scope counter все source taps отключаются и выдаётся resync.
Старые callbacks не получают права публиковать сообщения предыдущего scope.
Это отказ наблюдения, не завершение owner; после resync живая очередь возвращает
временную недоступность. Реальный exit появляется только при retirement.

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

The native unit fixture gates eight actual factory constructors immediately before
publication, for four rounds, then checks one recognizable winner, loser cleanup,
state/factory allocation refusal, retry and the lifetime of a real opened handle.
An independently relinked negative control restores only the old load/store
publication and must fail with `multiple published device factories`. Build errors,
timeouts and sanitizer reports do not count as detection. The same fixture also
retains its existing namespace/source races, queue and scope-isolation checks.

## Bound module scopes / Области привязанных модулей

An owner retains its base inbox plus independent native inboxes keyed by the exact
module binding identity. The device's native TLS selector uses the same nearest
bound live frame as [VFS lookup](VFS-TRAVERSAL.md), including builtin and unbound
helper calls. Returning to the caller therefore selects its own inbox for new
opens. No foreign process frames are read, no target pauses, and no guest callback
or registration protocol participates.

`FORKNS`/`NEWNS` prepare the selected inbox's new observer before committing the
namespace pointer. Failed allocation/subscription leaves the old namespace and
queue scope intact. A module's change must not rebind or purge the caller's base
inbox. Already opened FDs keep their captured inbox through return and reentry;
explicit pctl for that same scope still purges its old-scope records as before.
Two instances of the same module have different binding identities and queues.
Spawn creates its own observer rather than sharing the parent's consumable FIFO.

Очередь базового окружения и очереди привязок модулей независимы. Выбор выполняется
нативно по своим живым кадрам; возврат из модуля не перенастраивает открытые FD.
Смена namespace модуля затрагивает лишь его очередь. Отказ подготовки подписки
не фиксирует новую VFS; повторная попытка допустима. Ребёнок получает отдельного
наблюдателя, поэтому не отбирает события у родителя.

First native event-scope selection (device access/open or pctl) allocates the
module's slot and automatic observers; a bound child receives them before process
publication. There is no hook on every module-call boundary in this checkpoint.
Before a slot exists, provider journals supply conservative initial invalidation
when it is selected, not a replay of every earlier report. Subsequent delivery is
automatic even while execution is elsewhere. Native first-selection failure
refuses access; it never falls back to an unrelated base inbox. Slots contain no
managed module root and retire with the permanent owner; retained FDs can drain
the original queue after that retirement.

Слот модуля создаётся при первом нативном выборе области событий или pctl;
для наследуемой привязки ребёнка — до публикации потока. Отдельного перехвата
каждого входа в модуль пока нет. Ранние изменения представлены начальным
уведомлением о перечитывании состояния из журнала, а не полным воспроизведением.
После создания подписки доставка не зависит от исполнения получателя.

Acceptance adds actual interpreter/JIT module return/reentry, held caller/module
FDs, two same-path instances, real subscription allocation refusal with retry,
source publication while the caller executes outside the module, and a control
restoring the old wrong-inbox pctl argument. Native device tests
also cover explicit selector refusal without base fallback, bind-time reset and
an opened FD draining its original retired inbox through EOF.

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

[Subscription fault tests](vfs/test_sysevents_subscriptions.c) exercise unchanged
contexts/taps/watermarks, source/tap calloc refusal, unaffected-source push, later
native repair, removal and same-path replacement, reconnect/stale sessions,
forced epoch exhaustion and final native lifetimes. Source info/open callbacks
are forbidden. The guest fixture also checks that real Sys.chdir does not produce
fake provider changes. The concurrent source/namespace test now alternates100
rebinds with100 cwd mutations while another inbox changes groups200 times.
