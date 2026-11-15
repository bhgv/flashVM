# Effective provider stacks and indexed edits — runtime 0.133

Copyright (c) 2026 bhgv

The public [Vfsctl module](../module/vfsctl.m), implemented in
[pure Limbo](../limbo/vfsctl.b), uses the hosted scalar
[Vfsnative bridge](../module/vfsnative.m). It operates only on the calling resident's
namespace. There is no global provider registry, foreign PID parameter or new
nsadmin credential. Shared groups see changes; FORKNS creates an independent copy.

Vfsctl показывает effective stack конкретного пути, включая providers, унаследованных
от привязки родительской папки. Порядок — сверху вниз, как его рассматривает VFS.
Native anchored branches тоже отображаются как providers маршрута. Отсутствующий
файл в отдельной ветви не удаляет эту ветвь из списка: metadata status показывает
ошибку, а position сохраняет реальную позицию кандидата.

| API | Contract / Контракт |
| --- | --- |
| `init()` | Load the native service before sharing the module / Загрузить service до sharing |
| `layers(path)` | Status, coherent namespace revision, ordered list of copied Layer records; eight bounded stale retries / Согласованный revision и список всех кандидатов,8 retries |
| `layer(path, revision, index)` | One signed position; revision=-1 captures current;1/card,0/end,-2/stale,-1/error / Один кандидат и copied properties |
| `bind(source,target,flags,index)` | One transactional bind;0/success,-2/stale,-1/error; index only for MINS / Атомарная вставка, без replay effects |
| `move(path,revision,id,index)` | Move exact observed layer to final position; old revision refuses without effect / Перестановка по captured revision/ID |

Layer records contain group-local binding/head IDs, position/count, flags,
Native/Provider kind, metadata result, mode/directory, effective-access observations,
virtual mount point, capability-relative route and local provider journal
session/generation/capabilities. Absolute native source paths and native pointers
are not returned. IDs grant no authority outside the current namespace. An implicit
unmounted native route uses id0. Config counters outside signed-big range refuse
inspection; they never silently narrow. Source metadata is live, not an atomic
cross-provider snapshot or a promise that the next operation will succeed.

Access uses low bits X=1/W=2/R=4 and known-query bits shifted by8. Unsupported
access callbacks remain unknown. Metadata mode is advisory; actual operations
retain the final decision. Per-layer flags show MCREATE and MAUTORWX policies.
Classic Inferno traversal still selects the first existing resource; explicit
MAUTORWX selects the first permitted branch separately for read/write/execute.
Reordering does not introduce mid-chunk replay or automatic FD retargeting.

## Indices / Индексы

Positions are zero-based. Nonnegative indices count from the top; negative count
from the bottom. Insertions address gaps, while moves address final element positions.

| Stack size3 / Размер3 | `MINS` insertion slot / Место вставки | `move` final position / Итоговая позиция |
| --- | --- | --- |
| `0` | Before first / Перед первым | First / Первый |
| `1` | Between first/second / Между1/2 | Second / Второй |
| `3` | After last / После последнего | Invalid / Отказ |
| `-1` | After last / После последнего | Last / Последний |
| `-2` | Before last / Перед последним | Penultimate / Предпоследний |
| `-4` | Before first / Перед первым | Invalid / Отказ |

MINS=16 is an explicit extension used by Vfsctl.bind with its separate index
argument. Standard Sys.bind keeps its three-argument Inferno ABI and existing
MREPL/MBEFORE/MAFTER/MCREATE behavior; it refuses MINS. Vfsctl accepts those standard
order values too, plus MAUTORWX=8. MINS cannot combine with BEFORE/AFTER, and an
index for another order must be0. A multi-provider source is inserted as one
ordered block. File stacks are an explicit Vfsctl extension; classic Sys.bind's
BEFORE/AFTER directory-only restriction remains unchanged.

## File inherited from a directory / Файл из привязанной папки

Suppose `view` has directory layers A,C,B. Moving A to the top for `view/f` creates
an explicit file stack A,C,B at that exact path. The directory stack stays A,C,B
in its own prior order and `view/g` continues inheriting it. Moving a file does
not move the whole directory. A child-directory override works analogously.

A concrete example with a distinct starting order:

```limbo
include "sys.m";
include "vfsctl.m";
# sys and ctl are loaded modules; ctl->init() has completed.
sys->bind("a", "view", Sys->MREPL);
sys->bind("b", "view", Sys->MAFTER);
ctl->bind("c", "view", Vfsctl->MINS, 1); # A,C,B
(status, revision, layers) := ctl->layers("view/f");
if (status == Vfsctl->Ready) {
    c := hd tl layers;
    ctl->move("view/f", revision, c.id, 0); # f: C,A,B; view and g: A,C,B
}
sys->unmount(nil, "view/f"); # Remove override; f inherits A,C,B again.
```

This code fragment is an API sketch; the complete executable regression is
[main.b](fixtures/vfsctl/main.b), driven by [test_vfsctl.py](test_vfsctl.py).
An inherited-path edit captures the effective stack at that revision, assigns
child-local IDs, and publishes it atomically. Later parent changes do not mutate
that explicit override. Unmounting it resumes inheritance. Exact-stack moves
preserve existing layer/head IDs; new insertions get new IDs. Config changes are
pushed through the normal namespace system-event path, without receiver execution.

Existing open file FDs continue using their captured resource. Native directory
roots retain directory identity; files retain parent/name identity, matching the
existing bind model. Relative-name identity of provider files requires the declared
stable-bind contract. Existing live union directory cursors keep their documented
mount-head/index behavior; this does not create a frozen directory enumeration.

## Transactions and acceptance / Транзакции и проверка

[Native edits](vfs/stack.h) prepare all mappings/references before namespace commit.
Config revision, identity/index and overflow checks precede publication. Allocation
failure and stale revisions preserve the old stack. Native source metadata and
native directory acquisition happen outside namespace locks and VM admission.
Only scalar result publication retries; effects never replay. Inspect again after
stale instead of applying an old row number to a changed stack.

[Native tests](vfs/test_stack.c) cover signed boundaries including INT64_MIN, middle
and bottom insertion, exact IDs, stale edits, allocation rollback, file/sibling
isolation, copied namespace and restored inheritance. The guest tests execute
C/JIT with both GC observation settings, real native file I/O and a separately
installed readonly C provider. They verify all Layer properties, mixed file stacks,
provider session/capabilities, upper readonly read/lower writable write under
MAUTORWX, held FD identity, FORKNS isolation and final factory cleanup. Native
source bytes are checked independently from guest assertions.

No Sys.m, Dis ISA or upstream snapshot changes. This is the hosted NC64 extension,
not a claim that Inferno has indexed MINS or file unions. Standard Styx mount
transport and publication authorization remain separate APIs/work.

## Prepared publication / Подготовленная публикация

[Vfsexport/Vfspublish](VFS-PUBLICATION.md) add local typed capabilities for explicit
single-layer capture, immutable file/directory composition and combined mount/bind.
They use the same MINS/signed-position rules; Vfsctl continues to inspect and reorder
the installed providers. Capturing all nested binds is a separate future operation.
