# Frame action snapshots — v0.631

[`fvm_exec_call_frame`](../include/flashvm/execution.h#L342) is the public synchronous owner-thread execution service for
ActionCall (0x9e). It borrows a context-owned scope receiver (or uses `_root` for a
non-MovieClip receiver) and a valid [`FVM_Value`](../include/flashvm/value.h#L24) frame operand. A numeric selector
wraps to Uint32; other operands undergo AVM1 string coercion. Counted `clip:frame`
paths resolve before selecting actions. A missing target/frame is a no-op; unsupported
slash-only selector syntax returns [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L46). The private
[`fvm_movieclip_call_frame`](../src/standard/movieclip.cpp#L3383) receives an already resolved clip and numeric/string selector.
Neither route seeks the playhead, applies scene controls, emits onEnterFrame, polls
timers or advances children. Caller ownership of context/arguments lasts through return.

Previously direct native frame calls could restart the instruction counter for each
DoAction block, while the tick route had separate copying logic. Both now use
[`snapshot_frame_blocks`](../src/standard/movieclip.cpp#L2350). An absent `segments` field selects one legacy `code` value.
A present field is authoritative: it must refer to a same-context object containing
at most [`FVM_MOVIECLIP_MAX_FRAME_BLOCKS`](../src/standard/movieclip_constants.h#L23) (65536) indexed slots. Keys must be the dense
zero-based sequence 0..N-1; insertion order is irrelevant. An empty table is valid.
The old placeholder `code` field is not required when `segments` is present.
Snapshot all code values before entering the first block. A callback replacing or
removing the original table cannot change this captured batch; the next call sees
its replacement. Each block still has its independent AVM1 operand/local execution.

A null/foreign frame container, selected record or segment object, holes, unsupported
key ranges and oversize segment tables return [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45) before any
selected action runs. Code values are decoded at their execution turn: a bad type,
odd/invalid hex or invalid AVM1 in a later block may fail after earlier side effects.
[`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44), execution errors and exceptions propagate; earlier actions and
coercion effects remain committed. Tick may also have committed its playhead, scene
controls and sound notification before snapshot failure. No automatic rollback/retry.

[`FVM_ExecutionWorkScope`](../src/core/execution_work_scope.hpp#L9) shares the existing outer instruction/work budget across
operand coercion in the public service, target resolution, frame preparation and all
nested block executions. Only entry without an active execution/work scope resets the counter. A segmented
frame charges one unit per copied ordinal and one per entered block, plus ordinary
AVM1 checkpoints (including End). A legacy ActionCall charges one block-entry unit;
a legacy tick keeps its prior accounting. For selector strings, classify each byte
under one unit; a nonnumeric label search charges each stored frame record. Every
stored label is examined, and duplicate exact labels select the lowest frame index,
independent of insertion order. This changes native work counts for frame calls.

Zero instruction_limit disables its numeric ceiling, not existing cooperative
cancellation checks. This is bounded action-table preparation, not a new byte-wise
budget for hex decoding, every label comparison or every target helper. Existing
per-block bytecode bounds remain. Nonlocal slash-only syntax, full historical
case/version label conformance and original-Player reference acceptance remain open.

[GC:] [FrameScope](../src/standard/movieclip.cpp#L3429) links the copied values and receiver to the root chain, restoring
the debug cursor and nesting saved at frame entry on every return/exception. It deliberately
permits real script GC after a callback detaches the source segment object. The
existing tick native barrier retains its different GC restriction. No host callback
pointer is serialized; re-register host callbacks on a receiving context. The named
[`FVM_MOVIECLIP_CALL_FRAME_MAX_DEPTH`](../src/standard/movieclip_constants.h#L29) retains the existing 64-activation recursion bound.
This adds no concurrent-context access or new C ABI.

Tests cover direct/private and public native entry, interpreted and actual native
JIT ActionCall, tick, shuffled ordinal insertion, dense-key validation, exact work
boundaries, empty-block copying, nested/operand-coercion limits, counted duplicate
labels, allocation failures and transferred imported code after source destruction
and actual traced-string collection. The runnable [C89 example](../examples/embedding/movieclip/README.md)
executes frame two twice with separate five-unit limits and verifies playhead one
and zero tick callbacks. [Russian contract](FRAME_BLOCKS.ru.md).

From the independent component root, with the core's default AGG profile:

```sh
cmake -S . -B build-frames -DFVM_BUILD_EXAMPLES=ON
cmake --build build-frames --target flashvm_tests_frame_blocks_order flashvm_tests_frame_blocks_budget flashvm_tests_frame_blocks_invalid flashvm_tests_frame_blocks_empty_budget flashvm_tests_frame_blocks_gc_graph flashvm_tests_frame_blocks_allocation flashvm_tests_frame_call_labels flashvm_tests_frame_call_nested_budget flashvm_tests_frame_call_coercion_budget flashvm_tests_avm1_frame_call flashvm_examples_embedding_movieclip_main
ctest --test-dir build-frames -R '^(flashvm_tests_(frame_blocks_.*|frame_call_.*|avm1_frame_call)|flashvm_examples_embedding_movieclip_main)$' --output-on-failure
```

All nine roadmap stages remain open. These are FlashVM/Linux regressions, not
physical Android/Windows/macOS or original Flash Player conformance certificates.

## Validation for v0.631

196 selected Linux Release CTests passed with AGG/bundled static dependencies;
139 minimal whole-core ASan/UBSan CTests passed serially, with LeakSanitizer disabled
and no raster backend. Nine new frame regressions passed alongside existing
ActionCall/missing-operand/JIT dispatch and timeline/graph/input/timer tests. The
strict C89/pedantic-errors tutorial linked to the sanitizer core and printed all
five documented lines. Source-link and debugger-example checks passed.

A source-linked control using exact v0.630 movieclip.cpp reproduces the budget bug:
with limit 10, two six-instruction blocks complete with seen=2 and reported steps=6.
The current code returns [FVM_ERROR](../include/flashvm/flashvm.h#L43) with seen=1 and steps=11. AVM1 checkpoints count
the attempted instruction before rejecting it; native work rejects before increment
at the ceiling. This is FlashVM implementation comparison, not original Player proof.
No fresh SDL/device/foreign-OS run or full LeakSanitizer acceptance is claimed.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `fvm_exec_call_frame` | function | [include/flashvm/execution.h:342](../include/flashvm/execution.h#L342) |
| `FVM_ExecutionWorkScope` | class | [src/core/execution_work_scope.hpp:9](../src/core/execution_work_scope.hpp#L9) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_movieclip_call_frame` | function | [src/standard/movieclip.cpp:3383](../src/standard/movieclip.cpp#L3383) |
| `FVM_MOVIECLIP_CALL_FRAME_MAX_DEPTH` | macro / define | [src/standard/movieclip_constants.h:29](../src/standard/movieclip_constants.h#L29) |
| `FVM_MOVIECLIP_MAX_FRAME_BLOCKS` | macro / define | [src/standard/movieclip_constants.h:23](../src/standard/movieclip_constants.h#L23) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
| `FVM_Value` | struct | [include/flashvm/value.h:24](../include/flashvm/value.h#L24) |
<!-- source-links:end -->
