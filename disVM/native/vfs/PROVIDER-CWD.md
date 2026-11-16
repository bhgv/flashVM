# Retained provider cwd / Удерживаемый provider cwd (runtime0.164)

`Sys.chdir` now accepts a single provider directory as well as a native directory.
The guest ABI and provider callback tables are unchanged. The operation retains
a directory capability, rather than reopening its pathname for every relative
operation. A provider must support stable bind routing or fresh per-node capture.
Unsupported acquisition remains an explicit failure without native-path fallback.

`Sys.chdir` поддерживает одиночный каталог provider. После входа относительные
пути используют удержанный ресурс. Подмена исходного пути, covering bind и
unmount не заменяют cwd уже работающего потока. Это существующий Sys API,
а не новая команда или расширение Styx.

## Acquisition and commit / Захват и commit

Native acquisition resolves metadata, requires a directory and asks the provider
for effective search access when that query is supported. A stable route retains
its provider and relative prefix. A dynamic route captures a fresh root and uses
its dot identity. Metadata and supported search access are checked again on the
acquired resource; a capture that returned a file or refused access cannot become
cwd. Unsupported optional access queries do not manufacture rwx authority: the
provider's metadata/capture operations remain responsible for enforcement.

The resource plan owns staged roots until cleanup. Under the namespace mutex,
commit checks the original revision, cancellation, revision exhaustion and the
availability of a new group-local cwd subscription identity. It installs separate
cwd/import references together and increments the namespace revision once.
Displaced references retire outside the mutex. Failures preserve the previous
cwd and import base. Native callers must rebuild against the expanded
`DVM_ResourcePlan`; native callback-table and Styx wire layouts do not change.

Захват выполняется вне namespace mutex. После захвата проверяются тип и
поддерживаемый search-доступ; карточка metadata сама по себе не даёт прав.
Commit одновременно меняет cwd и import base. Ошибка, отмена или stale revision
не публикуют частично подготовленный каталог.

## Relative lookup, copies and events / Пути, копии и события

Relative paths within the cwd subtree keep the retained provider and prefix.
Covering replacements are ignored for that relative lookup. Explicit descendant
bindings still win, while absolute paths use the current namespace topology.
As with existing native-anchor cwd, normalized `..` outside the retained subtree
returns to virtual-root resolution. This does not implement a retained parent-Chan
walk across arbitrary renames or mount boundaries.

`FORKNS` copies native references and prefixes coherently under the source group
mutex. Its namespace topology and later cwd changes are independent; provider
contents remain live and shared according to the provider's own semantics.
Returning to a native cwd releases the old provider references.

Each resident automatically subscribes to its held cwd provider in addition to
visible mounted providers. A reserved group-local identity keeps this subscription
separate from mount entries. Origin-to-captured-root notification forwarding
continues after the original mount disappears. The existing `/dev/sysevents`
mailbox delivers one event per read, without Limbo watch registration, target
execution, cooperative polling or debugger pauses. Leaving the provider cwd removes
its subscription; source notifications cannot revive a departed cwd scope.

Копия namespace удерживает тот же ресурс независимо от дальнейших bind/chdir.
Изменения cwd provider автоматически попадают в `/dev/sysevents` даже после
снятия исходного mount. После ухода из cwd соответствующая подписка удаляется.
Формат и правила чтения событий не изменены: одно событие на чтение.

## Cancellation / Отмена

For eligible PATH/CAPTURE routes, `Sys.chdir` registers one native wait before
leaving managed execution. The scope covers metadata, search, capture, final-root
validation and commit. Cancellation stops subsequent callbacks and staged roots
retire through ordinary plan cleanup. A pre-commit cancellation returns failure;
an actual committed change keeps success if cancellation arrives afterwards.
A cancelled stale acquisition cannot start another retry. The source is retired
before managed reentry. Legacy providers and native anchors retain unscoped
admission and their existing blocking guarantees.

## Verification and limits / Проверки и ограничения

Fifteen native scenarios cover stable/fresh acquisition, search refusal and
cancellation, changed captured kind, missing capabilities, pre-cancel, capture and
metadata interruption, stale commit, commit cancellation, parent/descendant lookup,
module-path resolution, namespace copying and exact staged-root retirement.
Private omission of the commit cancellation guard is detected by the native test.

Actual interpreter/JIT guests cover source replacement, covering bind, unmount,
namespace copying, native cwd restoration, wrong-kind capture, file/union refusal,
provider event continuity and removal. Another guest test cancels/kills blocked
metadata, search, capture and fresh-root validation, plus a late-cancel commit
race. An independent native observer signals actual admission without sleeps.
All9 blocked jobs and all streams/factories must retire. A private missing-scope
control fails in the actual provider callback. Both guest tests pass ten repeats.

The native suite has269 cases; selected VFS/process coverage has28 cases and
host-unit coverage40 in each dynamic/static guest build.

**Still unsupported:** provider union cwd, `NEWNS` rooted inside a provider,
and module byte loading from a provider. Import-path resolution now retains the
provider, but the loader continues its explicit refusal; it does not open a host
file using the diagnostic source spelling. Ordinary native module loading works
after a return to native cwd. Full union cwd needs an owned mount-head/Chan
lifecycle; provider module loading needs provider-FD metadata, reads and cache
provenance. Neither is claimed by this release.

Пока не поддерживаются union cwd, NEWNS с корнем внутри provider и загрузка
байтов `.dis` из provider. Import base удержан корректно, но существующий loader
возвращает явный отказ. Следующие этапы — удерживаемый union mount-head/Chan и
provider FD в загрузчике; их нельзя заменить строками путей или снимком дерева.
