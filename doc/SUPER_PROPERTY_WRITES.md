# Super property writes / Запись свойств через super — v0.497

Loaded SWF4–8 super facades forward property watchers and raw own-membership
queries to the actual receiver. Membership is checked after the watcher returns,
including its mutations. If the receiver owns the requested data, native method
or virtual property, assignment through super does not modify or invoke it.
Hidden/readonly own properties still block ancestor virtual dispatch.

Without an own property, super searches the selected parent chain for a visible
virtual property, skipping intermediate data/native methods and hidden virtuals.
The setter receives the original receiver and transformed watcher value. A
getter-only virtual consumes the assignment. Canonical numeric, signed decimal
aliases and counted NUL names follow this same path; canonical numeric names no
longer return UNSUPPORTED in loaded contexts.

A loaded super facade has no writable local property storage. Missing parent
virtuals do not create receiver/facade data, change __proto__, consult __newindex,
or grow/truncate native Array.length. Host-only embedding retains its previous
named write behavior and canonical numeric-super rejection. Public C ABI remains
unchanged; only the private selected-write bridge gains a facade-write flag.

В загруженных SWF4–8 watcher вызывается на исходном получателе. После него
проверяется собственное свойство получателя: оно блокирует запись через super,
включая собственный setter. Если собственного свойства нет, поиск идёт по
родительской цепочке за обычными унаследованными данными к видимому virtual.
Setter получает исходный this. Getter-only virtual поглощает запись. Числовые
и NUL-содержащие имена поддержаны; обычное хранилище и Array.length не меняются.
Незагруженный host API сохраняет прежнее поведение.

The selected parent root is retained before the watcher, so detaching the
receiver prototype inside the watcher does not collect the captured chain.
The actual receiver, descriptor and suspended watcher exception remain rooted
through collection in callbacks. Own writes and searches finding no virtual propagate deferred watcher
exceptions; inherited virtual dispatch consumes that exception, while an exception
from the setter wins. Allocation failures unwind roots and permit retry; completed
user callback side effects are not transactional.

Tests: `super_property_write` covers SWF4–8, Object/Array, data/method/virtual
membership, readonly/hidden own slots, visible/hidden/getter-only/missing ancestor
virtuals, deeper data, named/numeric/signed/NUL keys and C API/interpreter/native
explicit/automatic JIT. It reproduces the v0.496 failure. `super_property_watch`
covers watcher-created/deleted properties, captured parent detachment, collection,
watcher/setter exceptions and getter-only virtuals. `super_property_allocation`
sweeps new/new[] failures, retries and traced strings. `super_property_transport`
serializes pure AVM1 setters and facades into another loaded context, then checks
counted strings, GC, dispatch and inert length writes.

Reference: Ruffle `2372a4e148a67d68efd796f4477b3cddd4f880cd`,
`core/src/avm1/object.rs` (`set_internal`) and
`core/src/avm1/object/script_object.rs` (`set_local`, `call_watcher`,
`has_own_property`). This is source comparison, not original Flash Player oracle
execution. Recursive property-call limits and the complete super/AVM1 model remain
open; no roadmap stage is closed by this change.

Validation: Linux Release minimal 997/997, AGG 1016/1016 and Cairo 1017/1017
have passing full-suite evidence after execute-bit repairs and targeted reruns
(7 minimal, 29 AGG, 6 Cairo binaries). Six restricted heap/global ASan/UBSan tests
pass: the four new super-write tests and existing super accessor/version checks.
Stack ASan and LeakSanitizer remain disabled in this environment. Foreign OS
runtime acceptance and full SWF conformance remain open.
