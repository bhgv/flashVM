# Versioned linkage casing — v0.380

Loaded SWF4–6 Object.registerClass names now use Flash's pinned UTF-16-unit
lowercase comparison. Registration, null/undefined removal, C89 registered-class
queries and native attach/duplicate class selection share the rule. Counted NUL,
lone surrogates and scalar/paired UTF-8 canonicalization remain supported. Dotted
I matches i; supplementary Deseret case pairs remain distinct because folding
works on units. SWF7–8 and unloaded hosts retain exact canonical spelling.

Replacing a legacy case-equivalent name changes both the constructor and stored
spelling. Erase/insert is staged and published by a nonallocating swap, so an
allocation failure preserves the previous spelling and constructor. Raw lookup
and comparison allocate nothing and invoke no getters. Constructor validation
still precedes name coercion. Public query failure retains its existing null
output policy; registerClass callback output follows the existing native-call
policy. There is no new public ABI, callback identity or graph format.

[GC:] The existing native call barrier protects staged constructor references
and reentrant name coercion. A transported Object registry keeps its spelling
and constructor graph. The receiver's current context selects comparison rules;
a transported library's fallback registry uses the same lookup when Object is
unavailable. Process-local prepared function bodies are still not transported.

Tests: object_register_class_case covers unloaded/SWF4–8 ASCII, Latin, Cyrillic,
Greek, dotted I, NUL, lone/paired surrogates, Deseret, replacement spelling,
removal, GC and independent graph transfer. object_register_class_case_allocation
sweeps replacement allocations, checks old/new state, GC/retry and proves raw
lookup under zero allocation allowance. movieclip_registered_case covers SWF5–8
export attachment, intrinsic duplication and transported fallback selection.
jit_registered_case compares registration/replacement/removal through interpreter,
explicit and automatic native CallMethod in host/SWF4–8.

Primary implementation comparison:
[pinned registry insertion/lookup](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/runtime.rs),
[registerClass argument validation](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals/object.rs).
This is source comparison, not an original Flash Player oracle pass.

Separate sensitive/insensitive global environments, exact mixed-movie/base-clip
version selection, preexisting case-colliding entries from a different context
policy, AMF typed aliases and full symbol binding remain open. Export asset
lookup itself is unchanged. All nine roadmap stages remain open.

Validation v0.380: all 789 AGG / 789 Cairo / 770 minimal Linux Release checks
have passing evidence after three ELF execution-permission repairs and targeted
reruns (one AGG, two Cairo; minimal passed initially). Eight restricted heap/
global ASan+UBSan checks pass: four new registry/scene/JIT cases plus existing
C89 registry, name/transfer, sprite resources and duplicate-version checks.
Stack ASan and LeakSanitizer remain disabled. No new original external SWF
oracle or foreign-OS run is claimed. All nine roadmap stages remain open.
