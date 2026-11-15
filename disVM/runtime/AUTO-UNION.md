# MAUTORWX: access-based union selection / Выбор слоя по правам

Copyright (c) 2026 bhgv

Runtime 0.127 adds an explicit extension requested for the hosted namespace.
`MAUTORWX` is bit8, declared in [vfsflags.m](../module/vfsflags.m).
It is not an Inferno flag. The pinned upstream `Sys.m`, signatures, operation IDs
and Dis format remain unchanged. `Vfsflags` contains constants only; do not load it.
Runtime 0.127 добавляет явное расширение: MAUTORWX=8 в vfsflags.m. Это не flag
Inferno; upstream Sys.m и guest ABI не меняются. Vfsflags — только константы.

```limbo
include "sys.m";
include "vfsflags.m";
# a, b and view are existing directories; check all return values in real code.
sys->bind("a", "view", Sys->MREPL | Vfsflags->MAUTORWX);
sys->bind("b", "view", Sys->MAFTER);
input := sys->open("view/data", Sys->OREAD);
output := sys->open("view/data", Sys->OWRITE);
created := sys->create("view/new", Sys->OWRITE, 8r666);
```

The extension applies to the entire stack if any current branch has MAUTORWX.
Order remains MREPL/MBEFORE/MAFTER. Removing the last flagged branch removes this
policy for future operations. FORKNS copies flags with the branches. Binding a
union as a source applies the new bind's flags: specify MAUTORWX explicitly on
that bind to enable the policy at the new target. MREPL without it restores the
[classic contract](UNION-NAMESPACE.md). More mode switches remain future work.
Расширение действует на всю стопку, пока есть хотя бы одна помеченная ветвь.
Порядок задают обычные flags. Удаление последней MAUTORWX ветви выключает policy
для следующих операций. FORKNS копирует flags. При bind union в новое место
MAUTORWX задаётся заново, явно. Остальные режимы и их переключение ещё обсуждаются.

| Operation / Операция | First eligible branch from the top / Первая подходящая ветвь сверху |
| --- | --- |
| `open(OREAD)` | Existing entry with effective `r`; a directory also needs search `x` / Существующий entry с `r`, каталог также с `x` |
| `open(OWRITE)` | Existing entry with effective `w`, without create/truncate / Существующий entry с `w`, без create/truncate |
| File module `load` | Existing non-directory with effective `r+x`, checked before cache lookup / Не-каталог с `r+x`, проверка до cache |
| `create(OWRITE)` | Parent directory with effective `w+x`; an existing entry also needs `w` and must not be a directory / Parent с `w+x`; существующий entry также с `w` и не-каталог |
| `stat`, bind/unmount validation | Ordinary first-existing metadata, not a guessed read selection / Обычный first-existing metadata |

In this explicit mode, creation uses the top eligible directory, even if that
entry is absent there but exists lower down. Lower entries are not changed.
MCREATE is not additionally required: MAUTORWX opts into directory-rights-based
creation. Outside this mode, Inferno's existing MCREATE behavior is unchanged.
`OEXCL` checks the selected target through the ordinary exclusive native open;
a collision fails without retrying a lower branch. It is not a global uniqueness
constraint across all layers. Supported ordinary Sys open/create flags stay the
same; this does not enable ORDWR or other previously unsupported operations.
В этом режиме create выбирает верхний подходящий каталог, даже если имя уже есть
только ниже. Нижние файлы не меняются. MCREATE дополнительно не требуется; без
MAUTORWX сохраняется Inferno MCREATE. OEXCL относится к выбранной ветви: collision
не вызывает retry ниже и не проверяет глобальную уникальность имён во всех слоях.
Набор поддержанных ordinary Sys flags не расширен.

## Rights and re-selection / Права и повторный выбор

Native permission selection uses
[`fvm_port_anchor_access_at`](../../flashVM/ports/anchor_port.h), implemented by
POSIX `faccessat(..., AT_EACCESS)` against the held directory descriptor. It checks
the effective host identity, including native supplementary groups and ACL policy,
rather than OR-ing the owner/group/other rwx bits. Paths still follow native
symlinks. Directory write without search is insufficient. Windows relative access
and a POSIX backend without AT_EACCESS return UNSUPPORTED, not guessed permission.
Native selection проверяет effective host identity через faccessat/AT_EACCESS от
held descriptor, а не объединяет rwx разных классов. Действуют native groups/ACL
и symlink rules. Write без search недостаточен. Windows relative access и POSIX
без AT_EACCESS явно UNSUPPORTED. Проверено на Linux AMD64; physical platform
acceptance остальных OS этим шагом не заявляется.

Trusted providers can supply the optional
[`DVM_ProviderOps.access`](vfs/provider.h) callback. It must report current effective
rights, including a dynamic read-only policy, without opening a resource or
performing effects. It can run concurrently on native callers, outside namespace
locks and managed admission. Missing/refusing access callbacks make a branch
ineligible in this mode; metadata mode bits never substitute for that authority.
This is an internal source interface, not a stable DLL ABI. Provider module
loading and provider create support are not fabricated by access selection.
Provider может добавить optional access callback: актуальные права, включая
read-only состояние, без open и эффектов. Callback должен поддерживать concurrent
native calls. Без него ветвь не считается разрешённой по metadata mode bits.
Это внутренний source API, не stable DLL ABI; access не реализует provider load
или create автоматически.

Every new open/create/load captures a stack and checks permissions again. A
permission/namespace change during selection remains a normal race: the real
open enforces access and can fail. After selecting, I/O executes once; an error,
quota, full buffer, invalid module, allocation failure or OEXCL collision does not
trigger lower-layer retry. Existing FDs/modules retain the selected resource;
individual read/write/seek calls do not switch it. No copying between layers,
concatenation, dynamic mode API, guest credentials or complete Chan.walk is added.
Каждый новый open/create/load заново проверяет права. Между check и open права
могут измениться: native open сохраняет полномочия и может отказать. После выбора
операция выполняется один раз; ошибки не вызывают retry ниже. Живые FD/modules
сохраняют ресурс, отдельные read/write/seek не переключают слой. Копирование,
склейка, dynamic mode API, guest credentials и полный Chan.walk сюда не входят.

## Acceptance / Проверки

[Guest fixture](fixtures/union-auto/main.b) and [host driver](test_union_auto.py)
use real independent chmod handshakes and host-byte checks for R/W/X selection,
owner-vs-other permission classes, parent write/search, missing/duplicate creates,
exclusive collisions, module-cache re-selection and retained FD/module behavior.
[Provider unit](vfs/test_auto_access.c) uses deliberately misleading mode=0777,
absent callbacks, changing rights, copied namespaces and a no-open guard.
Fixture проверяет реальную маршрутизацию bytes и модулей; provider unit отдельно
проверяет authority callback, отсутствие эффектов и lifetime.

For confirmed partial I/O and failure-only diagnostics after opening, see
[chunk results](CHUNK-IO.md). This does not change MAUTORWX layer-selection rules.
Для partial I/O после open см. контракт chunk results; выбор MAUTORWX не меняется.
