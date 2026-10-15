# Native method calls — v0.354

[Русское описание](JIT_METHODS.ru.md)

CallMethod and NewMethod use the shared rooted AVM1 method-dispatch service.
The native bridge preserves argument-count coercion before the nullish-owner
check, reversed stack argument order, prepared receiver selection, and the
host name-type restrictions. Loaded SWFs discard nullish-owner calls and
construction to undefined; host NewMethod retains the shared service error.
A nullish CallMethod discards arguments without allocating an argument copy.

Owner, name and arguments stay on the invocation's rooted operand vector while
coercions, property lookup, callbacks and constructors execute. The result
replaces the first consumed operand before the vector shrinks; this avoids a
new allocation after shared dispatch releases its temporary roots. Errors
propagate without committing the result. The existing safe helper boundary
translates allocation failures and restores native invocation state.

`jit_methods` compares host/SWF4–8 interpretation, explicit native bodies and
automatic native execution. It checks ordered arguments, callbacks collecting
garbage, thrown exceptions, active-code release rejection, primitive string
methods, null/undefined owners and a string argument count.
`jit_methods_allocation` injects invocation allocation failures through calls,
construction and collecting callbacks, checking GC frames, execution depths
and active native handles after every exit.

Named CallFunction/NewObject and function definitions still use interpretation.
These additions do not close any roadmap stage or supply other-CPU/OS runtime
acceptance. Public C APIs and the prepared-frame ABI are unchanged.

Linux Release: 715 AGG, 715 Cairo and 696 minimal checks pass after restoring
generated ELF execute permissions and rebuilding the corrected method fixture.
Both new tests pass restricted heap/global ASan+UBSan; stack ASan and
LeakSanitizer remain disabled by the existing environment restrictions.
