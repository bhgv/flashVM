# Extended Dis environment / Расширенная среда Dis

Copyright (c) 2026 bhgv

Compatibility with Inferno is the baseline, not a prohibition on extensions.
Every new extension must state its interface, semantics, lifetime, permissions,
format/ISA impact and acceptance scope. Existing behavior must not silently change
under the same interface. Missing Inferno operations must remain explicit refusals.

Совместимость с Inferno — основа, но расширения допустимы. Для каждого расширения
нужно явно описывать interface, semantics, lifetime, permissions, влияние на
format/ISA и границы проверки. Новое поведение не должно молча подменять прежнее.
Отсутствующие Inferno operations не имитируют успешное выполнение.

## Implemented here / Уже реализовано

| Interface / Интерфейс | Status and boundary / Статус и граница |
| --- | --- |
| [Native provider handles](PROVIDER-HANDLES.md) | Internal native callback contract for read/write/list; trusted host code only / Внутренний native contract, только trusted host |
| Stable-bind factory capability | Implemented internal extension DVM_PROVIDER_STABLE_BIND: fixed relative-name/node identity promise; absent flag refuses provider bind / Внутренняя capability, без неё bind отказывает |
| [Host provider installation](PROVIDER-NAMESPACE.md) | Internal namespace extension with references and revision checks; not public Sys.mount / Внутреннее расширение, не public Sys.mount |
| Provider-source Sys.bind(MREPL), directory MBEFORE/MAFTER/MCREATE | Ordinary matched Sys interface, provider-relative aliases with documented mutable-node identity limits; no new flags/opcodes / Обычный Sys, aliases provider-relative, без новых flags/opcodes |
| [Local guest publication capabilities](VFS-PUBLICATION.md) | Vfsexport/Vfspublish with exact native Root/Owner/Endpoint/Ticket/Request types, native caller identity and automatic VFS notices; no wire endpoint / Local capability API, не Styx server |
| [Prepared tree publication](VFS-EXPORT.md) | Native C SDK: request/owner selection/grant, PREPARE/direct/deferred modes and provider views; local guest adapter in0.135, Styx listener pending / Native/guest SDK реализован, wire ещё нет |
| [Vfsctl indexed layer control](VFS-LAYERS.md) | Implemented typed inspection/move/MINS and per-file override; separate from standard Sys.bind / Typed API своего namespace, standard Sys ABI не изменён |
| [MAUTORWX union policy](AUTO-UNION.md) | Implemented explicit bit8 via Vfsflags; effective R/W/X selection, parent w+x creation / Явное расширение выбора по правам, не Inferno flag |
| [Precise native chunk results](CHUNK-IO.md) | Implemented internal count/cause/certainty and failure-only info; heartbeat/Styx transport remains pending / Native API реализован, wire ещё нет |
| [Automatic per-thread system inbox](SYSTEM-EVENTS.md) | Native push and ordinary readonly /dev/sysevents, one record per read/readn; external format experimental / Native push без owner polling и pauses, формат предварительный |
| [Optional provider journal view](PROVIDER-EVENT-FILES.md) | Trusted host installation, independent diagnostic refresh/ack FDs; not required for automatic subscriptions / Optional view, не условие автоматической доставки |
| [Provider change journals](PROVIDER-EVENTS.md) | Implemented internal session/classes/resource hints and independent refresh cursors; no wire heartbeat or balancing / Native events реализованы, wire/balancing пока нет |
| [Native debugger endpoint](DEBUGGER.md) | Separate session-bound supervisor protocol; no guest address authority / Отдельный session-bound протокол, без полномочий по guest адресу |

Runtime0.127 additionally implements the explicit MAUTORWX extension.
Runtime0.126 adds ordered directory unions (see [contract](UNION-NAMESPACE.md)) and provider bind semantics, not a new Dis file format,
ISA operation or Sys.m signature. Compiler/runtime profiles remain explicitly
separate; see [DisVM architecture and profile status](../README.md). NC64 and
experimental Dis64 claims must not be merged into blanket binary compatibility.

Этот шаг добавляет provider bind, не новый формат Dis, ISA operation или Sys.m
signature. NC64/experimental Dis64 остаются отдельными явно описанными профилями.
Расширенная среда не означает автоматическую binary совместимость всех профилей.

## E1 — typed provider control / Типизированное управление

Runtime0.133 implements the local Vfsctl layers/layer/bind/move subset, including
signed MINS and per-file overrides. Runtime0.134 adds a native C publication/tree
composition SDK with capability-based grants. See [layers](VFS-LAYERS.md) and
[publication](VFS-EXPORT.md) for actual signatures, ownership and acceptance.
Runtime0.135 implements the local guest publication/capability adapter. Standard
Sys.mount over Styx and a network publication adapter remain proposals. No new Dis opcode, Sys.m frame or wire layout is assigned.

Runtime0.133 реализует local Vfsctl,0.134 — native C publication SDK. Public Limbo
публикация реализована в0.135; Sys.mount/Styx остаётся будущей работой.

Proposed requirements / Предлагаемые требования:

- Tokens bind session, generation, provider identity and permitted namespace scope;
  no native pointers or authority inferred from a PID / Tokens содержат session,
  generation, identity и разрешённый scope; без native pointers/PID authority.
- Read-only copied inspection happens on request, with bounded pages / Copied
  inspection выполняется по запросу, с bounded pages.
- Mutating requests use stable identities and distinguish accepted/completed/
  refused/unknown outcomes / Mutations имеют identity и явные состояния результата.
- Installation checks target kind, namespace authority and provider rights before
  publication / До publication проверяются target kind, authority и rights.
- Revocation/unmount affects new lookups; existing handles follow the declared
  lifetime contract / Unmount меняет новые lookups, старые handles живут по contract.
- DLL unloading requires all handles, callbacks and native jobs to retire; ordinary
  property removal is insufficient / Для unload должны уйти handles/callbacks/jobs.

This proposal does not authorize arbitrary guest-loaded code, introduce credential
groups or implement a dynamic loader. Those need their own explicit contracts and
checks. Until then installation remains host-only.

Предложение не вводит произвольную guest-загрузку кода, credential groups или
DLL loader. Для них нужны отдельные contracts и checks; пока installation host-only.
