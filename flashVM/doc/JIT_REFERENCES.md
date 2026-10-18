# Native variable and member operations — v0.350

[Русское описание](JIT_REFERENCES.ru.md)

The native compiler now lowers SetVariable, DefineLocal, DefineLocal2, GetMember,
SetMember, Delete and Delete2 into direct semantic helper calls. Every implemented
machine encoder uses the same call plan. This adds supported native operations;
it does not run an interpreter dispatch loop inside generated blocks.

| Operation | Shared service | Verification |
| --- | --- | --- |
| SetVariable | [`fvm_exec_variable_set`](../include/flashvm/execution.h#L408) | Plain/global writes, local lookup, qualified setter callbacks |
| DefineLocal / DefineLocal2 | [`fvm_exec_variable_declare`](../include/flashvm/execution.h#L428) | Prepared activation, retained initial value, undefined declaration |
| GetMember / SetMember | [`fvm_exec_member`](../include/flashvm/execution.h#L206) | Explicit object, loaded primitive boxing, collecting/throwing setter |
| Delete | [`fvm_exec_delete_member`](../include/flashvm/execution.h#L135) | Own member deletion and returned Boolean |
| Delete2 | [`fvm_exec_delete_reference`](../include/flashvm/execution.h#L391) | Binding/qualified deletion and versioned missing operands |

Native writes/declarations retain the interpreter's MovieClip-target/global
fallback, script-this and managed activation selection. Member and deletion
operands stay in the invocation's rooted vector until shared services finish.
Results replace existing operands without an allocation after temporary service
roots leave scope. Failures return through the native status epilogue. As with
interpretation, setter side effects and earlier writes are not rolled back.

`jit_references` compares actual interpreter/native invocation in host and SWF4–8
profiles. Native entry counters prove that attached bodies execute native code.
Missing operands are compared in fresh contexts: in SWF7 an empty Delete2 can
delete a versioned global `undefined` binding, so consecutive calls would start
from different states and constitute an invalid differential test.
`jit_reference_callbacks` collects inside script setters, exercises qualified and
explicit-owner writes, propagates thrown values and checks restored native/script/
AVM1/GC state. `jit_reference_allocation` walks invocation allocation failures
through all seven operations and checks inactive code handles and restored roots.

Six focused minimal-profile ASan/UBSan checks pass, including all three new tests
and existing jit_basic/jit_callbacks/jit_allocation. This environment cannot run
LeakSanitizer's `/proc` task inspection; leak detection and stack ASan are disabled.
Heap/global ASan and UBSan remain active. Sanitizer defaults are linked from an
uncommitted build-only object because child-process environment propagation here
does not reliably retain ASAN_OPTIONS. Generated configuration/logs stay outside Git.

Full Action support, optimizing JIT, real foreign-CPU/OS execution and complete
Flash reference equivalence remain open. See [JIT.md](JIT.md),
[JIT_ARCHITECTURES.md](JIT_ARCHITECTURES.md) and [ROADMAP.md](ROADMAP.md).

Linux Release validation: all 704 AGG, 704 Cairo and 685 minimal-profile checks
have passing evidence. Final suite runs needed focused reruns after restoring
execute permission on one AGG, two Cairo and seven minimal generated ELF files.
Those were launch failures, not behavior retries. Earlier development runs also
exposed and corrected the differential fixture's shared-state error described above.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_exec_delete_member` | function | [include/flashvm/execution.h:135](../include/flashvm/execution.h#L135) |
| `fvm_exec_delete_reference` | function | [include/flashvm/execution.h:391](../include/flashvm/execution.h#L391) |
| `fvm_exec_member` | function | [include/flashvm/execution.h:206](../include/flashvm/execution.h#L206) |
| `fvm_exec_variable_declare` | function | [include/flashvm/execution.h:428](../include/flashvm/execution.h#L428) |
| `fvm_exec_variable_set` | function | [include/flashvm/execution.h:408](../include/flashvm/execution.h#L408) |
<!-- source-links:end -->
