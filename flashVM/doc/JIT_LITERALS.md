# Native Array/Object initializers — v0.358

[Русское описание](JIT_LITERALS.ru.md)

InitArray and InitObject now lower to a native bridge using the shared count
normalizer and Array/Object construction services. All original entry operands
stay on the rooted native stack through count conversion, construction, name
coercion and property setters. Entry snapshots preserve the interpreter's pop
order. Results replace a consumed operand before shrinking the stack, without
allocating after the construction service releases its temporary roots.

Hosts require an integral numeric count and enough entry operands. Loaded SWFs
retain fractional truncation, NaN-to-zero, invalid signed-count rejection and the
existing 65536-entry materialization limit. Rejected signed counts replace only
the count with undefined and consume no entries. Missing loaded entries become
undefined; Object pair names and values are padded independently. Array entries
are own slots even for undefined values and bypass inherited setters/readonly
indices. Object entries retain ordered name conversion, duplicate-key assignment,
counted keys and the loaded policy of skipping empty converted names.

| Test | Coverage |
| --- | --- |
| `jit_literals` | Host/SWF4–8 interpretation, explicit and automatic native execution; stack order, duplicate/empty/counted-NUL keys, missing entries/pair members, rejected counts and exact consumption, fraction/NaN/infinity/string counts, materialization bound rejection, retained results after code release and GC |
| `jit_literals_callbacks` | Collecting/throwing count and key coercion; inherited Array accessors and readonly indices; native/activation/root restoration |
| `jit_literals_allocation` | Host/loaded failures through count/key callbacks, construction and publication; restored invocation state and retained result after code release/collection |

Callback side effects are preserved on failure, as in interpretation; property
writes and user callbacks are not transactional. Function definitions, With/Try,
remaining Actions and complete compatibility/runtime acceptance still require
work. No roadmap stage is declared complete.

Linux Release: all 726 AGG, 726 Cairo and 707 minimal checks pass after restoring
generated ELF execute permissions. All three literal checks pass restricted
heap/global ASan+UBSan; stack ASan and LeakSanitizer remain disabled.
