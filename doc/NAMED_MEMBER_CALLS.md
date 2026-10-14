# Named member calls / Вызовы именованных членов — v0.281

C89 `fvm_exec_invoke_member` accepts an explicit receiver, an already converted
counted name and positional values. It reads the member once and invokes that
value with the original receiver. Missing, scalar and noncallable values return
undefined. Ordinary member lookup retains the selected defining prototype before
a getter can detach it and collect. The selected function, receiver and copied
arguments stay rooted through the body; invocation metadata restores on failure.
Super receivers retain their existing dedicated dispatch policy.

Loaded CallMethod now uses this service for named calls. A missing member cannot
fall back to the interpreter's historical named-script cache. Anonymous loaded
calls on noncallable objects return undefined. NewMethod checks whether an object
is constructable before invoking it. Both actions use common primitive boxing,
including a bare receiver for unavailable global classes. Their existing ordering
is retained: argument preparation, receiver boxing, then method-name conversion.
Null/undefined original receivers still skip name conversion and return undefined.
Host-only cached fallback and native raw-table policies are preserved.

Именованный вызов читает член один раз и сохраняет настоящий receiver и прототип
владельца до вызова getter. Отсутствующий метод возвращает undefined и не вызывает
одноимённую функцию из старого кеша интерпретатора. CallMethod/NewMethod используют
общую упаковку примитивов; порядок преобразований и отдельная обработка super
сохраняются. Интерфейс принимает C-значения без байткода и стека интерпретатора.

Fixtures cover direct C89 calls, counted NUL, absent/scalar/noncallable members,
loaded missing classes, anonymous calls, stale named-script caches, real action
dispatch, a prototype-detaching script getter with GC, method arguments, exceptions
and allocation failures. Existing super, method-name and host method tests apply.
Full property case folding, super edge cases, constructor scalar-return policy,
mutable this and all roadmap stages remain open.

Primary reference: pinned Ruffle
[`activation.rs`](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/activation.rs)
`action_call_method` / `action_new_method` and
[`object.rs`](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/object.rs)
member-call dispatch. This checkpoint does not implement machine-code JIT.

Validation: each Linux AGG/Cairo profile passes 543 tests after execute-bit repair
and reruns for 1/4 BAD_COMMAND binaries respectively. Eight affected heap/global
ASan+UBSan checks pass (no stack/leak coverage). The getter fixture uses a null
absent setter, as required by addProperty; its initial undefined setter was
rejected before invocation. All 26 unchanged original SWFs match in both profiles.
