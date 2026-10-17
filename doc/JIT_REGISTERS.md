# Native registers — v0.352

[Русское описание](JIT_REGISTERS.ru.md)

ActionPush register operands and StoreRegister compile to native helper calls.
StoreRegister retains the top operand; it does not pop it. Mixed literal/register
Push records preserve operand order. Malformed register payloads are rejected
before a new code handle is published.

The prepared activation remains authoritative for register selection:

| Invocation | Bank and bounds |
| --- | --- |
| Loaded legacy function / zero-register Function2 | Context-global four-slot bank |
| Loaded Function2 with declared registers | Prepared private declared span |
| Host legacy function | Fresh private four-slot bank |
| Host Function2 | Existing host compatibility span, including its extra slot |
| Loaded root block | Persistent context-global four-slot bank |
| Host root block | Fresh rooted four-slot bank per root invocation |

Loaded out-of-range reads produce undefined and writes do nothing. Host private
out-of-range access returns INVALID_ARGUMENT. Missing StoreRegister operands use
the ordinary loaded undefined/strict-host policy. Global string stores use
[`fvm_exec_global_register`](../include/flashvm/execution.h#L461), retaining counted bytes/metatables and owning the
stored text before publication. Private banks are already rooted by the prepared
function activation; host root banks are now explicitly rooted in the native
root entry. No mutable bank is retained by immutable code handles.

Function2 preload construction remains in the existing shared activation builder.
Native code reads that prepared state instead of rebuilding this, arguments,
global or parameter preloads. The compiler still rejects function-definition
Actions; calling an already constructed Function2 can use native code.

`jit_registers` compares interpreter, explicit native and automatic execution for
host/SWF4–8, register round trips, retained stack operands, strings, bad indices,
empty stores, private parameter registers and this/arguments/global preloads.
It checks that private writes leave the global bank unchanged, root execution
publishes the same result and later functions observe the appropriate shared or
fresh bank. Native counters prove the tested paths enter native code.

`jit_register_gc` obtains a fresh object from a script getter, retains it only in
a register, removes its stack reference and calls another getter that collects.
It checks legacy and private Function2 banks plus native root entry. The object
is not pinned and the host's raw observation pointer is not a collector root.
`jit_register_allocation` walks private/loaded global string-store allocation
failures with embedded NUL and checks root/depth/active-code restoration.
`jit_invalid` checks truncated Push-register and oversized StoreRegister payloads.

These checks do not implement constant pools, function calls/definitions, all
remaining Actions, complete foreign-CPU/OS execution or final roadmap acceptance.
See [JIT.md](JIT.md) and [ROADMAP.md](ROADMAP.md).

Linux Release has passing evidence for all 709 AGG, 709 Cairo and 690 minimal
checks. Intermediate launch failures required execute-bit restoration; two Cairo
and one minimal generated executable were empty/corrupted and were explicitly
relinked before their successful reruns. No source test was bypassed. Nine focused
heap/global ASan+UBSan checks pass with stack ASan and LeakSanitizer disabled under
the existing environment restrictions. The suite also corrected setup of getter
descriptors and out-of-span Function2 parameters in the new fixtures; those setup
errors are distinct from register execution behavior. Actual non-Linux/foreign-CPU
VM runs and full roadmap acceptance remain unverified.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_exec_global_register` | function | [include/flashvm/execution.h:461](../include/flashvm/execution.h#L461) |
<!-- source-links:end -->
