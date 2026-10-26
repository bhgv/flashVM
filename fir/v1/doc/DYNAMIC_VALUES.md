# Dynamic values: implemented status and proposed Limbo boundary

[Coverage](COVERAGE.md) · [FIR types](FORMAT.md) ·
[Dis target](DIS_BACKEND.md) · [Русский](#по-русски)

| Layer | Current behavior |
| --- | --- |
| FIR | T_ANY exists for dynamic as.add and channel receive; supported SWF operations retain Flash semantics |
| Numeric source adapters | Bounded numeric profiles, not a general dynamic AS or Limbo type system |
| Our direct Dis backend | Rejects any/dynamic coercion before emission; supports explicitly typed i32/bool/f64 |
| Inferno compiler Tany | Internal polymorphic compatibility type; not a Flash dynamic-value container |
| Our Limbo source compiler | No Flash-style any syntax, constructors, operations or tagged-value lowering yet |

## Selected direction: channel-owned values

The agreed architecture is a channel-backed proxy to a value in the VM that
understands it. That VM owns the actual object, dynamic type rules and lifetime.
The other VM holds a checked endpoint/opaque object identity, not the object's
layout or a foreign heap pointer. A single owner router can serve many objects;
one process/channel per object is not required.

A local ADT/pick or native tag+union can still represent copied scalar replies,
errors and proxy descriptors. It is not the universal representation of every
remote object. Undefined and Null remain distinct. Number replies preserve
binary64 NaN/Inf/signed zero according to the selected conversion policy.

The existing broad Limbo syntax frontend recognizes ADT/pick syntax, but our
numeric FIR compiler does not yet lower these proxy types or general tagged ADTs.
This direction is a design contract; no Flash-compatible Limbo any, dynamic Dis
operations or new bridge wire ABI is implemented by this document.

## Proposed operations and their meaning

These are semantic operation names, not assigned opcode or wire numbers.

| Operation | Owner behavior |
| --- | --- |
| Describe | Return native type/class, exposed operations, conversion policies and current revision; do not invoke getters merely to enumerate capabilities |
| ReadAs(type, policy) | In one owner operation, validate and produce a copied value of the requested type, or an explicit conversion error; include actual source type and observed revision |
| Get(property) | Execute property access in the owning VM; return a copied scalar or another owner-backed proxy |
| Set(property, value) | Validate the request and apply the owner's assignment semantics; report completion separately from transport receipt |
| Invoke(method, arguments) | Execute the method once for this admitted request; preserve owner exceptions as structured error replies |
| Apply(operator, operands) | Execute dynamic operations in the VM that defines their semantics, including Flash +/equality/coercion rather than forcing both operands into Number |
| Retain / Release | Establish or retire a proxy's explicit remote owner; stale generations and revoked endpoints fail |

Describe answers what the owner exposes, not an unconditional promise that every
current value can be converted. For example, converting a string to Number can
succeed, fail or yield NaN depending on the requested policy. Native type,
requested output type and conversion policy are separate fields.

ReadAs is authoritative and atomic with respect to this owner request. Do not
implement it as client-side Describe followed by an unchecked read: the object
can change between requests. Expected revision may request a conditional read;
a mismatch returns an explicit stale-revision result. Owner evaluation must run
only once per request, especially for getter-backed values.

## Channel, lifetime and failure boundary

Within Dis, a typed request channel can name a proxy service. Across DisVM and
FlashVM, an adapter pumps copied protocol messages through the bridge. A raw
Limbo channel pointer is not serialized into the FlashVM heap. Replies carry
correlation ID, status, observed type/revision and either copied data, a new proxy
or a structured owner error. Type descriptions are metadata, not executable code.

Endpoint authority and object identity/generation are checked by the owner router.
The owner retains the target while an admitted request executes. New proxy
results establish their owner before reply publication; failed publication must
retire that owner. Release and endpoint shutdown revoke access without exposing
dangling foreign pointers. Garbage collection of one VM does not automatically
collect an object in the other VM; explicit remote ownership/leases and shutdown
cleanup must be implemented. A finalizer alone is not a lifetime protocol.

Neither UI nor VM owner may synchronously block its event pump waiting on a
request that requires that same pump. Requests/replies must be scheduled through
bounded queues. Getter/method callbacks and calls between VMs can be reentrant;
handlers need asynchronous continuation or explicit cycle refusal, not recursive
cross-owner blocking.

Transport acceptance is not execution completion. A timeout/cancellation after
admission can leave completion unknown; side-effecting operations must not be
blindly retried. Deduplication/cancellation policy, bounded outstanding requests
and response limits must be defined before implementation. A reported successful
Set/Invoke means execution completed, not merely that its bytes were queued.

Copied strings/scalars are snapshots. Cached values/type hints need a revision
or explicit immutable guarantee. Live proxies preserve owner access semantics.
Batching operations and executing whole expressions at the owner avoids one
cross-VM round trip per field/operator while preserving evaluation order.

The eventual serialized format must be versioned and independent of native C
union padding, pointer width and endianness. Its binding to the existing UI/value
transport remains implementation work; this document assigns no ABI numbers.

## Compiler implementation boundary

Start with an explicit proxy library and owner adapters. Only after their
ownership, conversions, errors and channel scheduling are tested should source
any/object accesses lower into that protocol. General source type refinement,
constructors and descriptor maps remain separate compiler work. Pointer/scalar
variants must never cause a Number payload to be traced as a heap pointer.

## По-русски

Принято направление: any и труднопереносимые объекты представлены **канальными
прокси к VM-владельцу**, которая понимает значение. Другая VM не получает его
внутреннюю структуру. Один маршрутизатор владельца может обслуживать много
объектов; отдельный процесс на каждый объект не обязателен. ADT/pick или C-union
остаются возможным представлением локальных скалярных ответов и метаданных.

Describe сообщает исходный тип, доступные операции/политики и ревизию. ReadAs
одним запросом возвращает значение нужного типа либо ошибку преобразования:
отдельные «узнать тип» и «прочитать» не гарантируют согласованность. Нативный тип,
желаемый тип и правила преобразования различаются; например, строка может дать
число, ошибку или NaN в зависимости от политики. Getter выполняется один раз.
Get/Set/Invoke/Apply исполняются у владельца, сохраняя семантику Flash-операторов,
свойств, исключений и преобразований.

Между VM передаются сообщения, значения и проверяемые идентификаторы/поколения,
не указатели Limbo-каналов или heap-объектов. Удержание объекта на время запроса,
владение новым прокси, Release/закрытие/отзыв и очистка после ошибки требуют явного
протокола. GC одной VM не освобождает автоматически объект в другой. UI/owner pump
не блокируется ожиданием работы, которая должна выполняться на нём же.

Приём сообщения не равен завершению операции. После тайм-аута допущенного запроса
результат может быть неизвестен; повторять побочные эффекты вслепую нельзя.
Нужны пределы очередей, корреляция, политика отмены/дедупликации и явная ревизия
кэшированных значений. Пакетные операции и выражения у владельца уменьшают число
канальных обменов. Формат сообщений должен быть версионированным.

Это архитектурное решение, не готовая реализация any: FIR T_ANY, внутренний
Inferno Tany и будущий канальный прокси различны. Реализация библиотеки/адаптеров,
типовых преобразований, lowering и GC-карт ещё предстоит.


## Implemented diagnostic and IDE foundation (v0.1598+)

The [native broker](../../../disVM/broker/README.md) now implements deviation-only
listeners, session fencing and exact source-origin records. Our numeric source
compiler records distinct read/write occurrences with lexical binding IDs;
those records do not introduce dynamic type lowering or automatic RPC emission.
The [IDE projection services](../../../ide/dis/PROJECTIONS.md) exercise explicit
reads/writes/method calls and stale-reply-safe watch rows on real Dis channels.
This bounded owner example is not yet generic any, network transport or a foreign
heap export/finalizer implementation. Global diagnostics do not emit success
messages. Timeouts retain unfinished owner records until completion is acknowledged.


## Explicit copied values and Limbo type reflection (v0.1605+)

The [IDE value contract](../../../ide/dis/VALUES.md) now separates scalar copies,
explicit table snapshots and Object/Method owner descriptors. `Idevalue.types()`
reflects the compiled Limbo profile, including classic/NC64 int width; `tofloat`
is an alias of `toreal`. Bool/Char remain explicitly marked extensions over int.
This is implemented in pure Limbo services and the optional AS adapter library;
it does not add native AS globals or general `any` lowering to our FIR compiler.
