# Function.call/apply superclass depth / Глубина super — v0.500

Loaded SWF4–8 Function.prototype.call/apply execute their target with superclass
depth one. The depth is independent of whether the function is stored on this,
a prototype or nowhere on the receiver chain. A super facade is still produced
when the receiver has no prototype; reads through its exhausted chain are undefined.
DefineFunction2 named/preloaded/suppressed super uses the same depth contract,
without changing register-count or paired preload/suppression rules.

A private invocation-depth field accompanies the selected function/owner metadata.
Call scopes save and restore it without allocation, including nested adapters,
exceptions and errors. Ordinary method/constructor scopes select owner-based
inference using a sentinel. Loaded adapters publish explicit depth only after
receiver boxing and apply argument gathering succeed. Noncallable targets and
super-as-call/apply-target keep their prior early-return policy. Unloaded host
adapters retain their previous owner-derived behavior. Public C ABI is unchanged.

В загруженных SWF4–8 call/apply задают глубину super, равную 1, независимо от
места хранения функции. При отсутствии прототипа фасад сохраняется, а чтение
его исчерпанной родительской цепочки возвращает undefined. Приватная глубина
сохраняется и восстанавливается вместе с контекстом вызова, в том числе при
вложенности, исключениях и OOM. Обычные методы и конструкторы продолжают
вычислять глубину по владельцу. Незагруженный host-режим не изменён.

Escaped facades retain numeric depth rather than a fixed prototype pointer, so
subsequent prototype changes remain observable after graph transport. Calling
the facade selects __constructor__ at its base depth and advances the constructor
activation's super depth. Existing wire format/validation already supports this
numeric metadata, including a depth greater than the currently available chain.
The scalar active depth adds no GC edge; existing inline call-root snapshots
retain saved/current function and owner identities. Receiver/capture roots remain
owned by the existing activation and call machinery.

Tests: `function_adapter_super` covers SWF4–8, call/apply, function storage at
multiple levels or absent, zero-to-three prototype links, C/member-adapter and
interpreter/explicit/automatic JIT entry, DefineFunction2 register/named/paired
flags (SWF7–8), escaped-facade GC and unchanged host behavior. It reproduces the
v0.499 failure. `function_adapter_super_scope` covers nested adapters, saved
outer metadata, parent detachment/GC, throws and errors. `function_adapter_super_allocation`
sweeps new/new[] failures and retry for missing-prototype receivers.
`function_adapter_super_transport` covers restored/live prototype chains and
constructor depth advancement through a transported facade.

Primary source: Ruffle `2372a4e148a67d68efd796f4477b3cddd4f880cd`,
`core/src/avm1/globals/function.rs` (call/apply invoke depth 1),
`core/src/avm1/function.rs` (load_super), and
`core/src/avm1/object/super_object.rs` (depth-based parent/constructor traversal).
This is source comparison, not original Flash Player execution. Complete AVM1,
property-callback super rules and foreign-platform release acceptance remain open;
no roadmap stage is closed by this change.

Validation: Linux Release minimal 1009/1009, AGG 1028/1028 and Cairo 1029/1029
have passing evidence after execute-bit repairs and targeted reruns (15 minimal,
28 AGG and 17 Cairo binaries; AGG's process harness also passed after its child
binary was made executable). Eight targeted restricted heap/global ASan/UBSan
checks pass: four new adapter-super tests plus borrowed/apply/super adapter and
super-conversion allocation controls. Both new/revised constants headers pass a
standalone C89 pedantic syntax probe. Stack ASan/LeakSanitizer and foreign OS
runtime acceptance remain outside this evidence.

Getter/setter callback depth is addressed separately in [v0.501](PROPERTY_CALLBACK_SUPER.md).
