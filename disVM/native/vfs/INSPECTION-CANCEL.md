# Layer inspection and move cancellation / Отмена просмотра и move (runtime0.163)

`Vfsctl.layer`, `layers` and `move` keep their existing Limbo signatures and
Inferno namespace semantics. Runtime0.163 registers native cancellation around
eligible provider I/O in these operations. Each guest keeps its own native thread,
Prog and REG; no guest event loop, debugger pause or global stop is involved.
Imported Inferno sources and the Styx wire format are unchanged.

`layer/layers` и `move` теперь допускают отмену ожидания поддерживающего provider.
Гостевой код не опрашивает очередь отмены и не приостанавливается отладчиком.
Права по-прежнему определяются provider и самой операцией; диагностическая
карточка слоя не выдаёт дополнительных полномочий.

## Inspection / Просмотр

A selected provider advertising `DVM_PROVIDER_CANCEL_PATH` receives one native
source covering raw metadata and the execute/write/read permission probes.
The wrapper registers the guest wait before leaving managed execution. It clears
the source and unregisters the wait before managed reentry or scalar publication.
Other providers and native anchors retain the existing unscoped path.

Cancellation stops subsequent permission callbacks, including when the interrupted
callback returned success. Its partial observations are discarded: `layer`
returns `Error` with no card. Cancellation takes precedence over a concurrent
namespace revision change and therefore cannot become `Stale`. The existing
`layers` wrapper discards its private list on `Error`; it neither advances to
another layer nor starts a stale retry for this interrupted inspection.

Without cancellation, provider errors and unsupported permission probes retain
existing diagnostic behaviour. A card can describe a metadata failure, and
unknown permissions remain unknown. A genuine namespace revision race still
returns `Stale`. Provider attributes are live observations, not an atomic remote
snapshot; the namespace revision fences configuration only.

При отмене неполная карточка или список не выдаются. После отменённой проверки
прав остальные rwx-запросы не выполняются. Даже если одновременно изменился
namespace, результат — `Error`, чтобы Limbo-wrapper не повторял прерванный запрос.
Обычные ошибки metadata и неподдерживаемые проверки доступа сохраняют прежний
диагностический смысл. Без отмены настоящий конфликт revision остаётся `Stale`.

## Move / Перестановка

The retained target stack is checked for PATH and, where fresh per-node capture
is required, CAPTURE eligibility. Eligible moves register one native source
covering the whole clone/capture transaction. Stable routes do not require fresh
capture. Legacy routes keep their existing behaviour; eligibility does not add
cancellation guarantees to a legacy callback.

An already requested source refuses the move before callbacks, even when the
requested position equals the current position. A descendant projection captures
its effective providers privately. Cancellation or refusal releases staged roots
and preserves the original stack and revision. The existing publication guard
checks cancellation under the namespace mutex immediately before commit.

An actual committed move remains successful if cancellation arrives afterwards.
There is no replay or compensating reorder. A noncommitted cancelled move returns
`Error`; a noncancelled revision conflict returns `Stale`. Signed indices, provider
order, layer identity rules and child-local projection semantics are unchanged.
Moving a child does not reorder the providers of its parent directory.

Перестановка готовится отдельно от видимого namespace. До commit отмена оставляет
стек и revision прежними; временные roots освобождаются. После состоявшегося
commit возвращается успех, даже если отмена пришла до возврата в Limbo. Перестановка
отдельного дочернего файла/каталога не меняет порядок родительской папки.

## Verification and remaining scope / Проверки и границы

The native move test covers successful two-provider projection, cancellation on
the first and final capture, pre-cancelled no-op/reorder, unsupported captured roots,
legacy capture refusal, unchanged parent order/revision and exact root retirement.
A private control removing only the initial move guard detects the erroneous
pre-cancelled no-op success.

The actual guest fixture runs interpreter and JIT in dedicated native-thread mode.
It cancels and kills blocked metadata, first/second/final permission probes and
move capture. It also covers final-root cancellation, committed-result races and
a concurrent revision change during `layers`. A native observer signals actual
callback admission; there is no timing sleep or guest polling. Counters require
all13 admitted native waits, streams and factories to retire exactly once, with
no later callback after cancellation. Ten repeated runs pass. A private control
removing wait registration fails in the actual native callback.

The full native suite contains268 cases and passes normally and with sanitizers.
Both dynamic/static guest builds pass26 selected VFS/process tests and40 host-unit
tests each. This release does not implement provider/union `chdir`: that needs a
retained cwd Chan across namespace copying, path resolution and module loading.
Legacy callbacks may still block; they must advertise and implement native
cancellation before receiving these guarantees.

Проверены interpreter/JIT, cancelwait/killwait, гонки commit и revision, отсутствие
повторного запроса и точное освобождение native ресурсов. `chdir` через provider
или union пока не реализован; native anchors сохраняют прежнюю поддержку.
