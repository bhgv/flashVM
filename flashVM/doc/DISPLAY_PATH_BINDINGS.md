# Display path bindings / Переменные путей display-объектов — v0.272

Loaded GetVariable no longer invents an implicit this value below SWF5. SWF4
timeline this is an ordinary variable: absent initially and readable after an
assignment. Existing function activation bindings remain available through normal
scope lookup. SWF5–8 implicit this precedence and unloaded host behavior remain
unchanged; mutable modern this cells are still follow-up work.

Concrete loaded MovieClips now resolve `_root` and `_parent` through the same
intrinsic-path mechanism as `_global`. Root/parent start in SWF5; global starts in
SWF6. Own fields/accessors precede these intrinsics, with prototypes following.
Class prototypes supply none. The loaded intrinsic fields are not own descriptors.
Parentless loaded property reads return undefined. The private preload/provenance
helper retains its prior null sentinel and bypasses replaceable script fields.

Legacy native root/parent prototype descriptors remain available in unloaded host
execution, including its existing read-only behavior. Loaded reads, membership
and ordinary writes ignore those compatibility descriptors; this permits a real
own binding rather than treating an inherited no-setter descriptor as a write
barrier. User-defined accessors with the same names remain visible.

[GC:] Intrinsic values are resolved from retained native scene edges without user
callbacks, validated before publication and retained by the existing operation
roots. Keyword matching uses counted ASCII spellings and the pinned Flash UCS2
fold table; exact matching no longer allocates a temporary comparison string.
Root/parent identities and lengths are documented macros in the common header.

`execution_display_path_bindings` exercises C89, unloaded/SWF4–8, child/root
provenance, parentless undefined, own binding and custom accessor behavior.
`avm1_swf4_path_bindings` covers the six ordinary SWF4 names from the original
swf4_vars prefix and modern root shadowing without changing raw preload identity.
`execution_display_path_bindings_allocation` injects failures through complete
GetVariable, member, qualified-path and With-scope reads, checking output/presence/
owner/root restoration.

This does not complete `_levelN`, ordinary case-folded properties, modern mutable
this, other display/system fields or complete stage semantics. swf4_vars remains
an unaccepted failing original until its system-property behavior is implemented.

В SWF4 `this`, `_root` и `_parent` не подменяют обычные переменные неявными
значениями. С SWF5 ссылки `_root`/`_parent` читаются из сцены после собственных
полей и раньше прототипов; у корня `_parent` возвращает undefined. Подменённое
поле не меняет исходную ссылку для preload. Старые нативные дескрипторы сохранены
в host-режиме, пользовательские getter не скрываются. Все стадии 1–9 открыты.

Primary reference: pinned Ruffle
[`activation.rs`](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/activation.rs)
`resolve`/`action_get_variable`,
[`script_object.rs`](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/object/script_object.rs)
own-property/virtual-property ordering, and
[`stage_object.rs`](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/object/stage_object.rs)
`resolve_path_property`.

Validation: Linux GCC13.3 AGG/Cairo each pass 523 tests, after restoring/rerunning
one AGG executable's execute permission. Eleven selected ASan/UBSan fixtures pass
(heap/global only; no stack/leak checks). All 26 accepted original SWFs match exact
traces in both profiles. The unchanged swf4_vars probe now emits the expected six
initial undefined values, then still stops at an unsupported system property;
it is recorded as failing, not added to the passing corpus.
