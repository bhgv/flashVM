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
| [Native debugger endpoint](DEBUGGER.md) | Separate session-bound supervisor protocol; no guest address authority / Отдельный session-bound протокол, без полномочий по guest адресу |

Runtime0.126 adds ordered directory unions (see [contract](UNION-NAMESPACE.md)) and provider bind semantics, not a new Dis file format,
ISA operation or Sys.m signature. Compiler/runtime profiles remain explicitly
separate; see [DisVM architecture and profile status](../README.md). NC64 and
experimental Dis64 claims must not be merged into blanket binary compatibility.

Этот шаг добавляет provider bind, не новый формат Dis, ISA operation или Sys.m
signature. NC64/experimental Dis64 остаются отдельными явно описанными профилями.
Расширенная среда не означает автоматическую binary совместимость всех профилей.

## Proposal E1 — typed local provider control / Предложение E1

**Proposed only; not implemented or accepted as a public interface.** A separate
module tentatively named Vfsctl could expose local native provider installation
through delegated capabilities. It would complement standard Inferno Sys.mount
for Styx/9P; it must not reinterpret Sys.mount frames or consume reserved flags.
No module ID, opcode, wire layout or final method signature is assigned here.

**Только предложение; public interface ещё не реализован и не принят.** Отдельный
модуль с рабочим именем Vfsctl мог бы управлять локальными native providers через
delegated capabilities. Он дополняет стандартный Inferno Sys.mount для Styx/9P,
не меняет его frames и не занимает reserved flags. IDs, opcodes, wire layout и
окончательные signatures пока не назначаются.

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
