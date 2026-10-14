# Shared Delete2 references / Общий обработчик ссылок Delete2

v0.257 moves Delete2 operand coercion and reference resolution into one C89 entry,
`fvm_exec_delete_reference`. The interpreter now checks operand availability,
selects its target, calls the service and stores the result. Inputs are managed
scope/target/this objects and a value: no bytecode, stack or raw local map is used.

The final dot/colon separates the deleted member. Its owner is resolved as a
variable reference: that prefix is parsed again using the SWF4 colon-only rule
or SWF5+ dot/colon rule. A bare slash prefix tries complete scoped object paths
in SWF5+, then a literal variable name if no object resolves. Plain references
use managed scopes and target/global fallback; exact this/_global handling is
included, while remaining historical name-case/this-cell semantics stay open.

`fvm_exec_scope_member` reads a final member only after checking its presence.
Missing members allow a complete retry in an outer scope; present undefined,
null or primitive values stop retries. Consequently an inner undefined owner
cannot cause deletion of an unrelated outer object. The final property being
deleted is still never read; DONT_DELETE/inherited deletion policy is unchanged.

This corrects v0.256's broader direct-object-path treatment. In particular, dots
inside an owner prefix are still parsed by GetVariable even after a slash: the
name `a/x.y:victim` does not simply resolve a child named x.y. The SWF4 owner
prefix `o.child` is a literal binding, while `o:child` is qualified. Unit fixtures
now assert these distinctions, including a real literal `o.child` binding. Direct
C object-path services continue supporting dotted clip names and parent paths.

Rules were checked against pinned Ruffle action_delete_2/get_variable and scoped
resolution, not against a modified SWF or changed reference output:
https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/activation.rs
Full Flash Player equivalence, generic GetVariable/SetVariable integration and
remaining legacy casing are not claimed.

[GC:] Every coercible name, converted text, original scope/target/receiver, owner
and string slice remains rooted across getter/coercion calls. Scoped member reads
retain their resolved object with an allocation-free frame. Getter exceptions
preserve outputs/presence flags; injected allocation failures restore frame chains
and do not delete the final property. Root slots and traversal bounds use documented
uppercase macros. Prefix getters may still perform their own visible side effects.

Delete2 вынесен из цикла в общий C-обработчик. Разделение имени выполняется в два
шага с учётом SWF4/5. При чтении владельца отсутствие свойства отличается от
существующего undefined/null/примитива: во втором случае внешняя область больше
не проверяется. Исправлена слишком широкая трактовка путей v0.256; собственные
фикстуры проверяют эти различия. Исходные SWF и эталонные трассы не изменяются.
Проверяются GC при преобразовании имени и в getter, независимый this, удаление
дескрипторов, исключения и отказы памяти. Полная совместимость и стадии 1–9 открыты.

Evidence: 476/476 Linux AGG and 476/476 Cairo tests; one AGG launch permission
failure passed after restoring its executable bit. Thirteen selected heap/global
ASan + UBSan checks passed (host stack/leak coverage remains disabled). All 25
unchanged original SWF traces matched in both profiles. Source-based unit checks
of versioned paths do not substitute for a full independent Flash Player corpus.
Проверено по 476 тестов двух профилей, 13 выбранных проверок памяти и 25 эталонов
в каждом профиле; ограничения инструментов и внешней сверки остаются явными.
