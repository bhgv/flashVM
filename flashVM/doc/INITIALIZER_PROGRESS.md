# Initializer progress and work — v0.632

Root DoInitAction remains the deferred SWF6–8 policy described in
[SWF_INIT_ACTION.md](SWF_INIT_ACTION.md): [`fvm_movieclip_load_swf`](../include/flashvm/movieclip.h#L93) validates/retains
code without executing it; [`fvm_movieclip_advance`](../include/flashvm/movieclip.h#L116) attempts library initializers
on the source root before scene/frame work, including when a shared descendant
is advanced first. ActionCall still executes only selected frame actions and does
not start initialization. Full Player preload/sprite-ID timing remains open.

Previously every later tick charged and copied the entire initializer table even
when its progress cursor had already reached the end. A completed library could
therefore make an otherwise empty stopped/single-frame tick fail under its normal
two-unit budget. The private [`run_initializers`](../src/standard/movieclip.cpp#L2483) helper now prepares only the pending
ordinal suffix; a completed table contributes no copy/entry work or code-snapshot allocation.
The ordinary node/optional-handler costs and later scene/frame/input work remain.

The whole table is bounded by [`FVM_MOVIECLIP_MAX_INITIALIZER_BLOCKS`](../src/standard/movieclip_constants.h#L27) (65536), separate
from the ordinary frame action bound. Validate same-context library/table/source
root edges before preparation, even when the cursor is already complete. For a
captured table size N and monotonic cursor P < N, copy ordinals P..N-1 into a native
value vector before attempting code. Missing pending ordinals reject the entire
new suffix with [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45); insertion order is irrelevant. Consumed
prefix entries are neither copied nor revalidated. A cursor at/beyond the current
size is complete; changing private metadata never automatically rewinds it.

[`FVM_ExecutionWorkScope`](../src/core/execution_work_scope.hpp#L9) charges one unit per copied pending ordinal, then one
before each attempted block, plus existing AVM1 checkpoints (including End).
The attempt cursor is committed before hex decoding/execution, retaining the
existing at-most-once failure policy. A decode error, [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44), exception
or instruction failure may skip a partly/unexecuted attempted block next time;
its earlier side effects cannot be rolled back. After clearing an exception, the
host may advance again to process the unattempted suffix. Capture failure or failure
to charge the next entry leaves that entry unattempted. Errors stop the current tick
before later scene/frame callbacks; earlier work from a batch stays committed.

For K pending blocks the extra native cost is 2K units and O(K) scratch, plus code
execution. A completed table has K=0. Limits remain work units rather than time;
zero instruction_limit disables the numeric ceiling, not existing cancellation
polling. AVM1 checkpoints may report limit+1 for the rejected attempted instruction;
native work refuses before incrementing at the ceiling. Neither hex decoding nor
all metadata operations gain a new byte-wise budget.

Callbacks cannot replace the captured batch. Changing an earlier/current slot does
not rewind progress. Appended ordinals beyond the captured batch wait for the next
initializer check; a later tick or shared descendant in the same outer host call
may observe them. This is a local invocation snapshot, not a whole-call freeze. A shorter/replaced table is interpreted with the same monotonic cursor,
not as a fresh library. The reentry flag suppresses duplicate initialization in a
nested advance, while that nested call may still do ordinary tick work; it is not
a whole-timeline reentry lock. The flag restores on success, error and allocation
failure. Library progress/flags are private runtime state and remain absent from
graph serialization: receiving contexts initialize their transferred library afresh.

[GC:] The existing outer tick [`FVM_NativeCallGuard`](../src/core/native_call_guard.hpp#L9) retains captured code and source
identity through table mutation. Explicit collection during native tick callbacks
returns [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45); actual collection outside the call remains supported.
No captured native identity escapes the call, no callback pointer crosses transport,
and the host re-registers its own callbacks in the receiving context. This does not
introduce concurrent access, public cursor editing or a new wire/C ABI.

Six new regressions cover completed root/child ticks in interpreter and actual native
JIT, exhaustive work boundaries, pending suffixes, mutations/reentry/failed-block
continuation, ownership/topology/caps, transferred source destruction and traced GC,
and allocation failures during partial initialization. Existing swf_init_action and
swf_init_action_allocation retain the earlier parsing/error/transfer coverage.
The runnable [C89 tutorial](../examples/embedding/movieclip/README.md) imports one
End-only initializer with a five-unit limit, then completes two later ticks in one
four-unit batch. [Russian contract](INITIALIZER_PROGRESS.ru.md).

From the independent component root:

```sh
cmake -S . -B build-init -DFVM_BUILD_EXAMPLES=ON
cmake --build build-init --target flashvm_tests_initializer_completed_budget flashvm_tests_initializer_pending_budget flashvm_tests_initializer_snapshot_mutation flashvm_tests_initializer_snapshot_invalid flashvm_tests_initializer_transfer_budget flashvm_tests_initializer_snapshot_allocation flashvm_tests_swf_init_action flashvm_tests_swf_init_action_allocation flashvm_examples_embedding_movieclip_main
ctest --test-dir build-init -R '^(flashvm_tests_(initializer_.*|swf_init_action.*)|flashvm_examples_embedding_movieclip_main)$' --output-on-failure
```

All nine roadmap stages remain open. These bounded FlashVM regressions do not
certify original-Player semantics, full lifecycle scheduling or foreign/device runs.

## Validation / Проверки v0.632

204 selected Linux Release CTests passed with AGG/bundled static dependencies;
147 minimal whole-core ASan/UBSan CTests passed serially, with LeakSanitizer disabled
and no raster backend. Six new initializer regressions and the existing init/action,
frame/graph/input/timer/native-JIT tests passed. The tutorial compiled under strict
C89/pedantic-errors, linked to the instrumented core and printed six expected lines.
Source-link, deterministic documentation and debugger-example checks passed.

A source-linked control using exact v0.631 movieclip.cpp fails an empty tick at
limit 2 after three initializers have already completed. The new code succeeds
at exactly two units, with the same three one-shot observations. This compares
FlashVM implementations; it is not original-Player or foreign/device acceptance.

Проверены Linux Release и минимальное ASan/UBSan-ядро; отключённый LeakSanitizer,
отсутствующий в нём raster backend и неисполненные SDL/foreign/device прогоны не
выдаются за дополнительную приёмку. Контроль старого кода воспроизводит расход
на завершённую таблицу; новый сохраняет три однократных эффекта и завершает tick
ровно за две единицы. Все девять стадий остаются открытыми.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ExecutionWorkScope` | class | [src/core/execution_work_scope.hpp:9](../src/core/execution_work_scope.hpp#L9) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_movieclip_advance` | function | [include/flashvm/movieclip.h:116](../include/flashvm/movieclip.h#L116) |
| `fvm_movieclip_load_swf` | function | [include/flashvm/movieclip.h:93](../include/flashvm/movieclip.h#L93) |
| `FVM_MOVIECLIP_MAX_INITIALIZER_BLOCKS` | macro / define | [src/standard/movieclip_constants.h:27](../src/standard/movieclip_constants.h#L27) |
| `FVM_NativeCallGuard` | class | [src/core/native_call_guard.hpp:9](../src/core/native_call_guard.hpp#L9) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
<!-- source-links:end -->
