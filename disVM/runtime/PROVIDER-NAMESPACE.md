# Retained namespace providers / Удерживаемые namespace providers

Copyright (c) 2026 bhgv

Runtime 0.124 adds a trusted **host-only** installation API. A namespace binding
may retain a [provider factory](vfs/provider.h) instead of a native directory
anchor. [Mount installation](vfs/resource.h) replaces an exact virtual prefix at
an observed namespace revision. A stale revision changes nothing. The host is
responsible for selecting and validating the intended target/kind; this internal
API is not Sys.mount and does not claim Inferno's complete mount admission policy.

Runtime 0.124 добавляет trusted host-only installation. Namespace binding может
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

Provider-backed chdir, provider-source bind, remove and module loading are
explicitly unsupported. Loader refuses before its native cache lookup. Neither
failure nor a diagnostic source label falls back to host path lookup. Native
resources retain their existing anchor paths. Filtered unmount compares provider
identity and relative selection; it does not confuse an equal display label or
native device/inode with another provider.

Provider chdir, provider-source bind, remove и module load явно unsupported.
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
| Provider-source bind, chdir, NEWNS inside provider / Provider directory operations | Explicitly unsupported; no host fallback / Явный отказ без fallback |
| Sys.mount, unions, Chan walk, Styx serving | Not implemented by this host API / Не реализованы этим host API |
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
HOST! decoy, unsupported operations leave decoys unchanged, and all 25 opened
states close before factory destruction. The driver runs interpreter/JIT and
both GC poll paths. Mandatory wide includes provider-namespace (72 workloads).
