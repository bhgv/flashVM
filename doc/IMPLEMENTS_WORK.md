# Budgeted interface declarations — v0.383

Loaded SWF7–8 fvm_exec_implements now shares native execution work with AVM1.
It charges entry, each captured interface input and each inspected input before
building the snapshot list. A complete count-N capture/inspection uses 2N+1
units, in addition to caller bytecode work. Nested calls retain the enclosing
counter; outer direct calls reset it. Budget exhaustion, counter saturation and
worker cancellation return FVM_ERROR. Zero instruction limit remains unlimited.
Unloaded host declaration accounting and its constructor-reference policy stay
unchanged.

[GC:] Declaration inputs remain rooted through snapshot preparation. Publication
still swaps a complete list only after all capture/inspection and allocations
succeed. OOM/budget failure leaves both edges and first-installation flag intact.
Repeated declarations keep the existing first list; they still validate and
process inputs under the existing declaration policy before deciding whether to
publish. Partial roots/snapshots unwind with execution depth restored.

Validated loaded zero-count, primitive-constructor and SWF4–6 requests now return
before allocating roots/snapshot storage or entering a work scope. Invalid input
values, foreign ownership, null argument arrays and the public count bound are
still checked first. This allocation-free claim applies to the direct C service;
bytecode operand adapters may allocate their own buffers. Input validation and
root storage initialization are not newly charged per unit, and the existing
65,536-argument public/decoder bound remains.

No getters, boxing or new coercions are introduced. Frozen/legacy edge formats,
first-empty-list behavior, ordinary membership rules, ABI and serialization
remain unchanged. Full Flash version/player and mixed-movie conformance remain
open.

Tests: implements_work covers SWF4–8, the full 65,536-reference public bound,
low/unlimited/saturated budgets, no partial installation, first-write retention,
GC and scope cleanup. implements_work_allocation sweeps publication allocations,
checks no-op success at zero allocation allowance, validates foreign values even
for noops and collects between failed attempts/retry. jit_implements_work checks
budget failure before publication and successful retry in interpreter, explicit
and automatic native modes, including SWF4–6 ignored declarations.

Primary comparison: [pinned ImplementsOp](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/activation.rs)
for argument/prototype/first-installation semantics. This change adds runtime
resource accounting; it is not a fresh original player oracle pass.
All nine roadmap stages remain open.

Validation v0.383: all 799 AGG / 799 Cairo / 780 minimal Linux Release checks
have passing evidence after five ELF execution-permission repairs and targeted
reruns (two AGG, three Cairo; minimal passed initially). Eight restricted heap/
global ASan+UBSan checks pass: three new declaration/work cases, existing
snapshot allocation/transport, membership data/large-graph and collecting
membership-callback regressions. Stack ASan and LeakSanitizer remain disabled.
No new original external SWF oracle or foreign-OS run is claimed.
All nine roadmap stages remain open.
