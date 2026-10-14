# MovieClip global scope / Пространство имён клипа — v0.270

A concrete loaded SWF6–8 MovieClip supplies its intrinsic `_global` value after
its ordinary own properties and before inherited/global-scope properties. This
applies when the clip is the explicit variable target or wrapped in a With scope.
Own undefined still shadows the intrinsic. SWF4–5 has no intrinsic namespace;
the class prototype is not a concrete display object and supplies none either.
Case spelling follows SWF6 folding and SWF7–8 exact matching.

This refines v0.269: its ordinary-object binding precedence remains valid, but
its generated concrete-MovieClip fixture incorrectly expected a global object's
own `_global` slot to precede the clip intrinsic. That assertion now expects the
intrinsic for SWF6–8, while preserving SWF4–5 and own-clip shadow checks.

Shared keyword recognition now lives in `scope_keywords.cpp/.hpp`, with counted
identities and Flash case-table columns documented in `scope_keyword_constants.h`.
No keyword code reads bytecode, interpreter stacks or local-frame maps. The C89
lookup/scope interfaces and unloaded host namespace policy remain unchanged.

`execution_global_movieclip` covers target/With behavior, inherited versus own
slots, undefined presence, spelling/version rules and class-prototype exclusion.
`execution_global_movieclip_allocation` verifies output/owner/presence metadata and
root restoration at every allocation boundary. Existing C/interpreter namespace,
this-case and callback/detached-receiver fixtures accompany the shared matcher.

Direct GetMember, qualified path and Delete2 namespace behavior remain separate
follow-up work; this checkpoint does not certify every display-property access.
The original swf4_vars system-property probe remains failing and unaccepted.

В SWF6–8 конкретный MovieClip предоставляет `_global` после собственных полей,
но раньше прототипа и внешней глобальной области. Это учтено для текущей цели
и `With`; собственный undefined сохраняет приоритет. Прототип класса не считается
экземпляром display-объекта. Сопоставление имён вынесено в общий модуль, а его
константы — в `.h`. Полные правила свойств, путей и все стадии роудмапа открыты.

Primary reference: pinned Ruffle
[`scope.rs`](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/scope.rs)
`resolve`,
[`script_object.rs`](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/object/script_object.rs)
`has_property`/`get_local_stored`, and
[`stage_object.rs`](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/object/stage_object.rs)
`get_property`/`resolve_path_property`.

Validation: Linux GCC13.3 AGG/Cairo each pass 517 tests. Seven AGG execute-bit
failures pass after restoration/rerun. Eleven selected ASan/UBSan fixtures pass,
including one successful execute-bit rerun (heap/global only, no stack/leak checks).
All 26 accepted original SWFs match exact traces in both profiles.
