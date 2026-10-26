# Channel projections in the IDE inspector

[Copied values and Limbo type reflection](VALUES.md)

[IDE](README.md) · [Bridge services](bridge/README.md) ·
[Broker diagnostics](../../disVM/broker/README.md) · [Example](examples/projection/README.md)

## Working slice

`bridge/ideprojection.m/.b` implements an explicit typed endpoint owned by a Dis
process. The initial owner exposes `number:int`, `text:string` and `add(int):int`.
It is a deliberately bounded object for testing the inspector contract, not a
universal object reflection engine. All reads, writes, enumeration and method
calls go through requests and replies. No client obtains the target ADT or native
heap address. A positive incarnation rejects stale target requests.

`bridge/ideinspect.m/.b` is a channel-owned view model for 16 watch rows. The
renderer selects a row/member, begins a refresh and receives a unique ticket.
A transport worker uses that ticket as the projection request ID. It forwards
the copied response with Apply; Snapshot returns a fresh row for rendering.
The view model never blocks on a target. A waiting worker does not prevent row
selection or another refresh, provided each model caller consumes its reply.

Tickets never recycle during model lifetime. Apply checks pending state, ticket,
response ID, owner incarnation, nondecreasing revision, status and text bound.
Late refreshes, duplicate replies and answers from a previous selection are
rejected. Copying rows prevents a renderer from mutating model state. Selection
resets displayed data, so the previous target's scalar cannot appear as current.
The renderer must display pending/status rather than interpreting an old value
as a newly completed read.

## Method invocation and pause

Describe returns declarations without evaluating getters. Read is an explicitly
typed field projection. Invoke requires `permit == 1`; expanding a row never
executes a method. This consent bit prevents accidental invocation by the client,
**not** unauthorized network access. The adapter still needs real authentication
and capability policy before exposure to untrusted clients.

Pause here is cooperative owner state: reads/description remain available, but
writes and methods return Paused. It is not proof that arbitrary heap inspection
is safe during a real debugger suspension. A future debugger adapter must publish
safe snapshots at VM safe points while its broker remains independently runnable.
Release replies once and exits the owner; callers must retire its endpoint and
must not send more requests. There is no channel-close/GC-finalizer protocol in
this example. Never retry a timed-out mutation automatically: invocation deduping
and cancellation acknowledgement belong to the future transport/owner adapter.

The service serializes `add` at the owner, avoiding a client read-modify-write
race. It does not yet implement a general expression transaction such as
`a = y - a + 8 * a`. Integer arithmetic follows the selected Dis target.

## Checked now, still to connect

The acceptance example runs actual spawned Dis owners and real typed channels,
with native dispatch counters checked in interpreter and AMD64 JIT modes. It
checks Unicode, read/write/method result, explicit invocation consent, stale
incarnation, independent objects, pause/resume, release, stale refresh, duplicate
response, changed selection and copied row isolation.

This series supplies IDE services/model and packaged modules; it does not yet
add a graphical watch panel, serialize these requests over a socket, bridge them
to an arbitrary FlashVM object, or lower Limbo `any` in our FIR compiler. The
existing C broker/BD1 codecs provide the independent diagnostic layer, but the
projection sample does not pretend its Dis rendezvous is a network session.
The modules currently compile with the matching source-built Limbo bootstrap;
our Coco/R FIR frontend still has a bounded numeric profile.

## По-русски

Инспектор уже может работать с канальными проекциями в виде отдельных Dis-сервисов:
прочитать/изменить поле, раскрыть описание объекта, явно вызвать метод владельца.
Модель строк написана на Limbo; все команды идут через каналы. У каждого обновления
свой билет. Ответ от предыдущего объекта, старого обновления или повторный ответ
не меняет текущую строку. Отрисовка получает копию, не ссылку на состояние модели.

Проверены настоящие Dis-процессы в интерпретаторе и JIT. Кооперативная пауза
разрешает чтение, но отклоняет запись и выполнение метода. Это ещё не адаптер
полной остановки процесса отладчиком. Сетевой RPC, общий Flash-compatible `any`,
автоматический GC экспортированных объектов и графическая панель остаются
отдельными шагами; текущий пример не подменяет их симуляцией.


## Explicit copied values and bound methods (v0.1605+)

The owner/model now depend on `idevalue.m` and must rebuild together. ReadAs
returns a typed copy or Object/Method reference. Convert accepts a value argument,
including a constant; WriteValue stores only a copied matching scalar. ToTable
is an explicit shallow snapshot with nested owner references. InvokeRef validates
the bound receiver/incarnation before running its method. The model detaches both
incoming and rendered typed records. See [the value/type contract](VALUES.md).
