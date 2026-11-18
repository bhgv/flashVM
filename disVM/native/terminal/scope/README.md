# Nested S-record namespaces / Вложенные пространства S-записей

`styx_terminal_scope` adds an explicitly selected native broker profile. A root
or group owns a namespace of envelope tags. Each envelope contains a complete
sequence of child S-records. A client receives the records inside its envelope,
without receiving its parent tag, root service stream or transport handle.

The same local tag can therefore appear independently in different clients and
groups. A native host creates the tree and grants independent RX and TX tags.
No global application tag allocation is required.

RU: Внешние S-записи принадлежат брокерам. Клиент видит только записи своего
вложенного пространства. Одинаковый ID разных клиентов не конфликтует; их
внешние конверты выбирают адресата. Root/group handle и client inbox — разные
opaque API-типы, application не получает native authority родителя.

## Namespace boundaries / Границы пространств

| Record | Recipient |
| --- | --- |
| Transport root tag0 | Native root service inbox only |
| Group-local tag0 inside its envelope | That group's native service inbox only |
| Client-local tag0 inside its envelope | That client's local record, never ancestor control |
| Client-local tag7 in two unrelated envelopes | Their two separate client FIFOs |
| Valid-looking S-record bytes inside an opaque client payload | Unchanged payload, no recursive guessing |

Routing follows only explicitly created groups. Client envelopes are decoded once
into local messages; leaf payload bytes are opaque. The profile does not scan
arbitrary payloads for control commands. Unknown nonzero outer/group tags,
incomplete inner frames or damaged checksums refuse the entire affected outer
transaction and retire the scoped broker with PROTOCOL.

Every outer payload in this profile is a complete sequence. Network/carrier
fragmentation of outer records is handled by the existing native control decoder.
Splitting a single inner frame across separate outer envelopes is not supported
by this profile. Empty envelope sequences contain no messages; an empty inner
payload remains one client message. Native nesting is bounded to 32 levels and
all encoded ancestors must fit the configured record limits.

## Native host and client APIs / Native host и client API

[`scope.h`](scope.h) distinguishes `StyxScope` native host authority from
`StyxScopeClient` application inbox/send capability. Host pointers are local
native capabilities, never serialized pointers or guest heap objects.

```c
/* control is owned; this explicit constructor consumes it even on failure. */
StyxScope *root = styxbidirscopes(control, 4096);
/* Check each allocation/result before use; host retains root through all calls. */
StyxScopeClient *a_inbox;
StyxScopeClient *b_inbox;
StyxScope *a = styxscopeclient(root, &a_inbox);
StyxScope *b = styxscopeclient(root, &b_inbox);
StyxScopeAssignment incoming[] = {{a, 11, 0}, {b, 12, 0}};
/* IDs11/12 are illustrative already-agreed parent tags, not reserved production IDs. */
int assigned = styxscopeassign(root, STYX_SCOPE_RX, incoming, 2);
```

Applications receive only their inbox capability. `styxscopeclientwait` blocks
that consumer until its own queue has data, retires or reaches its native timeout.
`styxscopeclientnext` copies exactly one local tag/body. SMALL changes only the
required size and preserves the head. Root/group services have separate native
`styxscopeservicewait`/`styxscopeservicenext` APIs.

The sole broker blocks for incoming outer records and dispatches copied messages
into dynamic recipient FIFOs. It never waits for a client to consume messages,
executes guest callbacks or pauses guest execution. Root retirement wakes every
independent client/service wait. Already accepted records remain individually
drainable before END. Child handles live until their root is freed; hosts retire
all external users before rootfree joins its broker and owned control workers.
Local root disposal leaves the shared Styx client with its host.

## Dynamic assignment / Динамические назначения

`styxscopeassign(parent, direction, list, count)` atomically changes a set of
sibling assignments. Zero unbinds a child; nonzero tags must be unique in that
parent's final namespace. One transaction can swap tags without a temporary
collision or gap. Each entry carries the exact expected **local** generation.
Stale decisions, duplicate entries, foreign/non-child handles, final collisions
and generation overflow refuse the whole transaction without partial changes.
`styxscopebinding` copies the current native tag/generation.

The entire outer-envelope admission transaction holds one mapping lock. It
resolves recipients, validates nested frames and stages payload copies before
publishing any of them. A malformed later child or allocation failure frees
all staged copies; no earlier prefix of that outer transaction leaks into an
inbox. Previously committed outer transactions remain readable. Real allocation
failure reports NOMEM, distinct from a slow recipient.

An accepted message already belongs to its resolved inbox. Reading it after a
remap does not re-resolve the current outer tag. Local generations are not wire
fields, authentication or distributed barriers.

[Confirmed native operations](operation/README.md) now provide ordered root-service
handling, ACK-gated TX proposals and atomic RX commit plus reply admission. Only
affected tags wait; unrelated clients remain live. The native adapter must still
select an agreed wire encoding, authenticate requests and handle receiver replay.
No numeric negotiation command schema is invented here.

For a single ordered connection, old records precede the assignment request and
new affected-tag records wait for its ACK. Multiple striped carriers need order
restoration for that tag, not a global drain of unrelated channels. The proposed
64-bit record sequence and separate 64-bit assignment generation are selected;
these fields are not implemented on the wire yet.

## Outgoing authority / Полномочия отправки

RX assignments never create TX authority. The native host separately assigns
TX tags for every outgoing ancestor. `styxscopeclientsend` takes a client-local
tag/body, creates a complete inner S-record and wraps it through those explicit
TX ancestors. Its nonblocking return means whole-record transport acceptance,
not remote execution. A missing ancestor grant, excess encoded size or full
transport TX staging refuses admission without replay or channel shutdown.

The encoded path is copied before acceptance. Later TX remapping cannot rewrite
an already queued frame. Client-local zero remains inside its wrappers and cannot
turn into a transport root tag-zero command. Native client code has no API for
selecting arbitrary parent tags or sending raw unwrapped channel bytes.

## Integration scope / Границы интеграции

This is a separate native profile. Existing flat binding/UI/resource routes are
not silently reinterpreted. The same original S-record codec, captured Styx grant,
RemotePhy carrier and native control transport are reused. No new listener,
network-specific carrier, interpreter, graphics library or wire opcode is added.

Runtime `$Unibus` attachment, automatic selection of this profile, agreed
production root-service wire codec, remote `sysevents` forwarding and
scope-aware pressure diagnostics remain explicit integration work. This native
layer provides the isolated inboxes, complete envelope transport and atomic local
assignment needed for that work; it does not claim those integrations already run.

## Verification / Проверка

Thirteen [original-Styx scenarios](tests/host.c) run real bidir pumps and native
libstyx server/client operations with three-byte reads and two-byte write ACKs.
They check duplicate local7 across clients/groups, independent client waits,
client-local zero, native root/group zero, opaque nested-looking payloads, SMALL,
512 unread messages, RX swaps/stale/conflicting/foreign assignments, frozen TX
paths, explicit directional grants and nested local-zero TX.

Other cases corrupt a later inner frame, refuse the second staged malloc,
use an unbound outer tag, corrupt a later group envelope, shut down the original
client and stop the root. Constructor faults exercise subscription refusal,
service-sender and broker-worker creation refusal, sticky terminal replay and successful private retry.
An outer-body boundary case uses a scoped limit smaller than control staging and
checks that outer tag bytes are charged alongside payload.
A late-callback case deliberately delivers admitted notices after rootfree.
The optional C++98 fixture links public headers without private scope layouts.
The peer's fixture commands and receipts establish test activation barriers;
they are not production negotiation schemas or remote application execution.

Negative controls deliberately leak client-local zero into root services, publish
a staged prefix after malformed input, or infer TX grants from RX. Each mutation
fails its corresponding original-Styx assertion in the new fixtures.
