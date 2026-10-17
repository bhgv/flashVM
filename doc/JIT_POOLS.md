# Native constant pools — v0.353

[Русское описание](JIT_POOLS.ru.md)

ConstantPool, Push-constant8 and Push-constant16 compile to direct native helpers.
Each invocation starts from its prepared function's captured pool. A root entry
starts with no inherited pool. The public prepared-frame ABI is unchanged: a
private scoped view passes the activation's already-owned byte-string snapshot
to native dispatch and restores the caller view on all exits.

Each ConstantPool record is copied into an immutable code-owned replacement set.
A helper selects a set only when that Action executes. Pool references read the
invocation's current set at runtime; they are not replaced by the lexically
nearest ConstantPool during compilation. Forward skips and backward jumps can
therefore observe the same pool state as interpretation. Sharing native code
between identical bodies never shares captured pools or mutable invocation state.

Out-of-range/missing references return undefined in loaded SWFs and
INVALID_ARGUMENT in host execution. An empty pool clears the prior selection;
it does not fall back to captured entries. Referenced text is adopted into the
context before entering the GC-rooted operand stack, so returned strings outlive
code handles. Like the interpreter's existing own_string helper, inherited pool
entries use terminated-byte semantics. This does not extend pool syntax to
embedded-NUL string entries.

Compilation checks exact record lengths, terminators and operand widths. Total
entries across replacement sets are bounded independently by
[`FVM_JIT_MAX_POOL_ENTRIES`](../src/jit/constants.h#L7) (65536). This implementation limit is separate from
the UI16 per-record count/length limits and from primitive literal storage.
Failures preserve the caller's previous code handle.

| Verification | Evidence |
| --- | --- |
| `jit_pool` | Host/SWF4–8; interpreted, explicit and automatic native execution; captured/replaced/empty/missing pools; forward/backward jumps; index 256; mixed literals; one cached body with different captured pools |
| `jit_pool_callbacks` | Nested native function and fresh root entry inside a collecting/throwing getter; caller-view restoration; returned text after code release |
| `jit_pool_allocation` | Compile/invoke allocation failures; transient pool, GC frame, activation and active-handle restoration |
| `jit_pool_bounds` | Malformed/truncated records and indices; exact aggregate limit and rejection beyond it |

Ten focused heap/global ASan+UBSan checks pass, including native script bridges
and diagnostic replay paths. Stack ASan and LeakSanitizer remain disabled under
the existing environment restrictions. The fallback fixture in execution_modes
now uses StringLength, which is still unsupported, instead of ConstantPool.

All Push operand formats are now represented in the native subset. This does not
mean full Action coverage: function definitions/calls, object/array construction,
With/Try and remaining operations still require interpretation. No foreign-OS/CPU
runtime acceptance or roadmap stage completion is inferred from these checks.

Linux Release: 713 AGG, 713 Cairo and 694 minimal checks pass. AGG/Cairo
test objects were rebuilt for the changed private context layout. Generated
ELF execute permissions were restored for AGG/minimal launch failures.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_JIT_MAX_POOL_ENTRIES` | macro / define | [src/jit/constants.h:7](../src/jit/constants.h#L7) |
<!-- source-links:end -->
