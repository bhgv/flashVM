# Retained namespace providers / Удерживаемые namespace providers

Copyright (c) 2026 bhgv

Runtime 0.125 extends a trusted **host-only** installation API. A namespace binding
may retain a [provider factory](vfs/provider.h) instead of a native directory
anchor. [Mount installation](vfs/resource.h) replaces an exact virtual prefix at
an observed namespace revision. A stale revision changes nothing. The host is
responsible for selecting and validating the intended target/kind; this internal
API is not Sys.mount and does not claim Inferno's complete mount admission policy.

Runtime 0.125 расширяет trusted host-only installation. Namespace binding может
удерживать provider factory вместо native directory anchor. Mount заменяет
exact virtual prefix при совпадении revision; stale отказ ничего не меняет.
Host отвечает за выбор и проверку target/kind. Это не Sys.mount и не полная
Inferno mount policy. Guest не получает native callbacks/указатели.

## Lookup and ownership / Поиск и владение

[Resolution](vfs/resolution.c) applies the existing longest component-prefix
selection and retains the chosen provider under the namespace mutex. Metadata
and open run outside that mutex and outside managed admission. The factory sees
a normalized relative suffix and an existing native mode spelling. A failed open
must leave its output empty. A successful open returns one independent valid
handle. The wrapper allocates before invoking the factory and retains factory
state/code until after the inner handle's consuming close.

Resolution сохраняет longest component-prefix и удерживает provider под mutex.
Metadata/open выполняются вне mutex и managed admission. Factory получает
normalized relative suffix и mode; failure оставляет output пустым, успех
передаёт independent valid handle. Wrapper выделяется до callback и удерживает
factory/state/code до consuming close внутреннего handle.

The namespace, copied namespaces, in-flight plans and opened handles own separate
references. Rebind/unmount affect new lookups; retained selections and open FDs
keep their original provider. Last reference runs nonblocking native destroy
outside locks. Callbacks must not access managed memory, reenter namespace or
invoke guests. This is not dynamic DLL unload support.

Namespace, copies, in-flight plans и opened handles имеют отдельные references.
Rebind/unmount меняют новые lookups; старые plans/FD сохраняют provider. Last ref
вызывает nonblocking native destroy вне locks. Callbacks не обращаются к managed
memory/guest и не входят обратно в namespace. Dynamic DLL unload отсутствует.

Provider-backed chdir, remove and module loading are
explicitly unsupported. Loader refuses before its native cache lookup. Neither
failure nor a diagnostic source label falls back to host path lookup. Native
resources retain their existing anchor paths. Filtered unmount compares provider
identity and relative selection; it does not confuse an equal display label or
native device/inode with another provider.

Provider chdir, remove и module load явно unsupported.
Loader отказывает до native cache lookup. Нет fallback к host path по diagnostic
label. Filtered unmount сравнивает provider identity/relative selection, а не
совпавшую display-подпись или native device/inode. Полный provider walk, union,
Styx/9P mounts и guest credentials остаются следующими этапами.

## Inferno compatibility / Совместимость с Inferno

Compatibility is a requirement. Public Sys.m, frame layouts and guest operation
IDs are unchanged in this step. Semantic references are the pinned Inferno
experiment snapshot in [UPSTREAM.json](../UPSTREAM.json), revision
7f2241455734e1142a3254fad9d1ec4f94032ac7: cclose/cmount/cunmount in
[emu/port/chan.c](../upstream/emu/port/chan.c), plus Sys namespace entry points in
[emu/port/inferno.c](../upstream/emu/port/inferno.c). Upstream files remain unchanged;
this host adapter does not import whole-OS scheduling or stopped-world sections.

| Contract / Контракт | Current scope / Текущая граница |
| --- | --- |
| Open resource survives unmount / Открытый ресурс переживает unmount | Preserved and tested / Сохранено и проверено |
| Shared namespace; FORKNS detaches copy / Shared namespace и отдельная копия | Provider references inherit/copy with the group / References наследуются и копируются |
| MREPL native bind / Native bind MREPL | Existing supported subset retained / Прежнее подмножество сохранено |
| Provider chdir, NEWNS inside provider / Provider directory operations | Explicitly unsupported; no host fallback / Явный отказ без fallback |
| Directory unions / Union каталогов | [Runtime 0.126 supported subset](UNION-NAMESPACE.md) / Подмножество с явными границами |
| Sys.mount, Chan walk, Styx serving | Not implemented by this host API / Не реализованы этим host API |
| User/group/rwx / Права пользователя | Native host credentials and provider factory policy; no guest credential groups / Host credentials и factory policy; guest groups отсутствуют |

These tests run the hosted VM, not two complete Inferno runtimes. Source-reviewed
semantic agreement is not blanket binary/device/security compatibility. Future
public provider APIs must follow the matched Inferno interfaces and receive
explicit compatibility tests; missing operations must not fabricate success.

Тесты исполняют hosted VM, не две полные Inferno-системы. Сверка source semantics
не означает общей binary/device/security совместимости. Будущие public provider
APIs должны следовать Inferno interfaces и получить отдельные compatibility
checks; отсутствующие операции не имитируют успех.

## Checks / Проверки

[test_provider_mount.c](vfs/test_provider_mount.c) checks stale install, coherent
copy, rebind with identical labels, old versus new metadata, unmount and an open
handle pinning its factory after all namespace/plan owners are gone.

[test_provider_namespace.py](test_provider_namespace.py) installs through the
actual host mount API at startup, without per-FD substitution. Eight native
guests copy namespaces and exercise ordinary read/write/list Sys operations.
A retained parent FD survives filtered unmount, a new open reads the original
HOST! decoy, unsupported operations leave decoys unchanged, and all 30 opened
states close before factory destruction. The driver runs interpreter/JIT and
both GC poll paths. Mandatory wide includes provider-namespace (72 workloads).


## Provider bind in 0.125 / Provider bind в 0.125

Ordinary Sys.bind(MREPL) now captures provider identity plus the resolved relative
source, after the same metadata/kind and revision checks used by native binds.
It does not recursively store the source alias name. Directory-root bindings
preserve suffix lookup; subtree/file aliases preserve their relative prefix.
Replacing or unmounting the original source does not redirect an existing alias.
Alias-to-alias binding captures the final factory/relative selection. Filtered
unmount compares those identities, not display labels.

Sys.bind(MREPL) сохраняет provider identity и resolved relative source после
прежних metadata/kind/revision checks. Имя source alias не становится рекурсивной
ссылкой. Directory root сохраняет suffix lookup; subtree/file сохраняют relative
prefix. Rebind/unmount исходного source не меняет готовый alias. Alias-to-alias
захватывает конечный provider/relative selection. Filtered unmount сверяет их.

The guest acceptance includes file, directory-root, subtree and alias-to-alias
binds, rejected kind mismatch, alias survival after source unmount and removal of
all aliases with 30 exact close callbacks. Native unit checks stale bind refusal
and frozen aliases through rebind. This uses the existing Inferno Sys interface;
local native provider registration remains a separately documented extension.
See [implemented extensions and proposal E1](ENVIRONMENT-EXTENSIONS.md).

**Compatibility limit:** the current factory API captures a provider-relative path,
not a retained Chan/node identity inside a mutable provider. Rebinding the source
namespace alias is safe, but a provider that renames/replaces its own nodes could
change future opens through that relative path. Such providers need stable walk/
node handles before full Inferno bind identity can be claimed. The present memory
provider fixture has immutable relative names; it does not prove rename semantics.

**Граница совместимости:** пока сохраняется provider-relative path, а не retained
Chan/node внутри изменяемого provider. Namespace source rebind не перенаправляет
alias, но внутренний rename/replacement provider-node может изменить следующий
open. Для полного Inferno bind identity нужны stable walk/node handles. Текущий
memory fixture имеет immutable relative names и не подтверждает rename semantics.


Runtime0.125 therefore gates provider-source bind on the explicit internal native
capability DVM_PROVIDER_STABLE_BIND. A trusted factory may declare it only if
relative names keep the same resource identity throughout factory lifetime.
Without that promise, bind refuses until a retained walk/node interface exists.
Open/stat and host installation remain available without this flag. Unknown
factory flag bits refuse creation without consuming caller state. This extends
the native factory contract, not Sys flags, Dis opcodes or a stable DLL ABI.

Поэтому bind разрешён только с внутренней capability DVM_PROVIDER_STABLE_BIND:
trusted factory гарантирует неизменную relative-name/resource identity до конца
lifetime. Без promise bind отказывает; open/stat и host installation остаются
доступны. Unknown flags отвергают создание без поглощения caller state. Это
расширение native factory contract, не Sys flags, Dis opcodes или stable DLL ABI.

Runtime 0.126 combines bindable provider directories with native branches using
[ordered union bindings](UNION-NAMESPACE.md). Earlier milestone limitations above
remain historical; the linked contract defines current union behavior.
Runtime 0.126 объединяет provider/native ветви; текущие границы union — по ссылке.

Runtime 0.127 adds optional effective-access callbacks for [MAUTORWX](AUTO-UNION.md).
Absent callbacks never infer permissions from provider mode bits.
Runtime 0.127: optional access callback; без него mode bits не дают MAUTORWX прав.

Runtime 0.129 attaches a [native change journal](PROVIDER-EVENTS.md) to each factory.
Aliases/copies share the source journal; each observer owns its refresh cursor.
Runtime 0.129: journal принадлежит factory, observers не очищают чужие cursor.
