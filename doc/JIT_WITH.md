# Native lexical With blocks — v0.371

ActionWith now compiles its body into an owned native child mapping. The parent
and child run direct semantic calls and machine branches; this path does not
invoke the AVM1 interpreter. Unsupported child Actions reject the entire native
compilation, allowing the existing automatic whole-body interpreter fallback.
Both standard external bodies and the historical inline encoding are accepted.

The child shares its parent's rooted operand vector and prepared register bank.
It also inherits the currently selected constant pool and returns pool changes
to the parent, including through errors and Return. A source offset base preserves
function-relative debugger locations. Child checkpoints share the parent's work
counter/poll cadence. An empty or implicit-end With block has an uncharged exit;
an explicit End is a charged source Action. Return propagates through all enclosing
With blocks and leaves the complete function, without charging synthetic exits.
Ordinary branches cannot cross block boundaries or target an implicit child exit.
Trailing bytes after explicit End are now rejected, matching interpreter framing.

A null/undefined operand is popped and skips the body. Other values use the shared
With-scope builder, including loaded primitive boxing when a class is unavailable.
The managed lexical node is published before dropping the input operand. Scoped
cleanup restores the enclosing chain on success, Return, script exceptions and
allocation failure. Function definitions inside the block capture this node and
retain its object after the parent returns and its code is released.

Parent handles own and recursively release child mappings. Both mappings remain
active during child execution, so collecting/reentrant callbacks cannot release
them. `fvm_jit_code_size` reports emitted bytes in the complete owned tree; automatic
cache accounting and the compilation byte limit include those child bytes. The
nesting limit is the interpreter's 64 With levels. Compilation failure leaves the
caller's existing output handle unchanged and frees all prepared child mappings.
Children are nested blocks within one native entry, not separate script calls.

| Test | Coverage |
| --- | --- |
| `jit_with` | Host/SWF4–8, interpreter/manual/automatic, external/inline and explicit/implicit End, shared operands/pool/global and Function2 registers, nested Return, nullish/missing operands, boxing/missing class, exception recovery, closure-only ownership after parent release and traced GC |
| `jit_with_validation` | Malformed records/trailing bytes, invalid cross-boundary/implicit-exit branches, unsupported child and whole-body fallback, 64 accepted/65 rejected levels, exact instruction budgets and child source offsets, cached body breakpoint exclusion and native reentry |
| `jit_with_callbacks` | Collecting/throwing getter and subsequent independent collector while only the With scope retains its object; active parent/child release rejection; restoration of lexical chain, roots, pool and execution depths |
| `jit_with_allocation` | Every compile/invoke allocation boundary for nested inline/external With, host/loaded and manual/automatic entries; unchanged output handle, recursive inactive-code checks, exact aggregate cache-byte accounting and successful retry |

Try, audio/quality Actions and full Flash conformance remain open. The evidence
compares FlashVM's interpreter and native paths; it does not establish full
reference-player behavior or real Windows/macOS/Android runtime acceptance.
All roadmap stages remain open.

Linux Release: all 763 AGG, 763 Cairo and 744 minimal checks have passing evidence
after ELF permission repairs/reruns, including the minimal reference-runner child.
Six targeted checks pass restricted heap/global ASan+UBSan; stack ASan and
LeakSanitizer remain disabled. All dependent test objects were rebuilt for the
private JIT layout changes. Fresh fixtures pass 26 synthetic Unicorn ISA/ABI cases.
