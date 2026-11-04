# Continuous source and int-field protection for LENC

The startup-only `--guarded-copies` profile selects the same guarded C handler
from the interpreter and native x64 tail. Default retained LENC is unchanged.
This is a native-cell memory protocol, not a shared Dis64 wire/Frame64 extension.

`dvm_runtime_string_length(source,destination)` requires exact ACTIVE admission
and independently live pointer/WORD slots. It guards their canonical six-key
union, captures the pointer, and adds exactly sizeof(int) at String.len without
releasing any original guard. After capturing this field it sign-normalizes into
WORD and publishes. H means length0; NULL means exNilref. INT_MIN and invalid
slot/field geometry give exHeap, as the retained native opcode does. A negative
Rune count is logical characters, not UTF-8 bytes. Adjacent String.max is not read.

Every error preserves output and ends all claims before returning the static
error to the VM. Extension BUSY/refusal leaves its original access live, so the
handler explicitly ends that access. Unknown/prohibited admission returns exHeap
before payload access. No allocation, retain/root creation, safepoint, callback or
object-owner park occurs inside. Existing native metadata mutex fallback remains;
this is not a wall-clock bound.

Caller source/referent/container lifetimes and existing collector exclusion are
independent preconditions. Continuous source exclusion stabilizes the captured
owner against compliant slot writers; the discovered field guard excludes
compliant length writers. Neither numeric keys nor refcount1 establish allocation
provenance, uniqueness, type, root validity or permission for concurrent GC.
General managed execution still uses NativeWorld. Mutable string builders and
other readers/writers need their own compatible protocols before its removal.

The actual heap fixture covers both newstring/newrunes allocations for lengths0..31,
7680 unaligned/overlap source/output cases, precise pointer/WORD/int union widths,
canaries, field/output alias with complete metadata restoration, H/NULL, wrapping
geometry and INT_MIN. Genuine source owners are independently rooted and balanced;
no fake Heap headers/native keys enter GC. A real caller-rooted string survives32
Cycles before another guarded read and final Fast cleanup. Source/field/publication
omission controls fail at exact stages; BUSY preserves output and releases all.
The actual x64 entry/cache/unmap/error oracle exercises both poll modes. Tests
must be run against matching object closures; C alias cases alone are not evidence
of parallel managed admission.

```sh
python3 disVM/runtime/test_shared_string_length.py RUNTIME LIMBO disVM/upstream/module
python3 disVM/runtime/test_string_length_tail.py RUNTIME LIMBO disVM/upstream/module
```

`--positive-only` runs instrumented positives without deliberate semantic defects.
Generated machine code is not instrumented by TSan. There is no new physical
ARM64/RV64 or general parallel collector proof. Performance costs and unmeasured
candidates are retained under STRINGLEN-104 in PERFORMANCE-COSTS.md; percentages
across different guest workloads must not be summed.

[Runnable commented example](../examples/guarded-string-length/README.md).
[Remaining managed-heap gates](HEAP-PARALLELISM-STATUS.ru.md).
