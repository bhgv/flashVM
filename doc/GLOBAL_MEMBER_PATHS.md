# Namespace members and paths / Пространство имён в свойствах и путях — v0.271

Concrete loaded MovieClips now expose intrinsic `_global` through ordinary
member reads as well as variable/With lookup. Ordinary own data, methods and
accessors precede the intrinsic; inherited prototypes follow it. The intrinsic
is not reported as an own property. Native MovieClip instances used as prototypes
also supply it when their place in the chain is reached. Class prototypes remain
excluded; SWF6 folds the namespace spelling and SWF7–8 use exact spelling.

Qualified scoped member reads report virtual presence without pretending that
the intrinsic is an own descriptor. Object path reads share the namespace policy,
including loaded SWF6 availability. When a path explicitly starts at the context
global object, ordinary bindings (including undefined) precede its fallback alias.
The unloaded host path alias retains its previous reserved-name behavior.
Delete2's missing-prefix fallback now uses the same version/case matcher.

`execution_global_member_paths` covers C89 direct members, has-own distinction,
qualified path services, inherited native display objects, own undefined,
explicit global versus MovieClip starts, custom namespace bindings and Delete2.
`execution_global_member_paths_allocation` checks three service paths and output/
presence/root restoration on allocation failures. `avm1_global_member_paths`
exercises real member, GetVariable and Delete2 action dispatch. Existing generated
path fixtures now expect the implicit namespace only from SWF6, rather than SWF5.
Independent SWFs and oracle bytes are unchanged.

This does not complete ordinary property-name case folding, all display-object
virtual properties, removed-clip behavior or full path/metadata equivalence.
The original swf4_vars system-property probe remains an explicit failure.

Общее правило `_global` применяется при чтении членов, составных путях и Delete2.
Неявное свойство видно, но не становится собственным полем; свой undefined
сохраняет приоритет. Прямое начало пути в глобальном объекте учитывает обычные
привязки, а host-режим сохраняет прежний alias. Имена, версии и Unicode-регистр
распознаются общим модулем без доступа к p-коду. Полная совместимость и стадии
роудмапа ещё не завершены.

Primary reference: pinned Ruffle
[`activation.rs`](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/activation.rs)
`get_variable`/`resolve_target_path`,
[`script_object.rs`](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/object/script_object.rs)
`get_local_stored`/`has_own_property`, and
[`stage_object.rs`](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/object/stage_object.rs)
`resolve_path_property`.

Validation: Linux GCC13.3 AGG/Cairo each pass 520 tests after rebuilding/rerunning
the corrected generated path fixture; one AGG execute-bit failure was restored.
Twelve selected ASan/UBSan fixtures pass (heap/global only, no stack/leak checks),
after an initial execute-bit failure was restored. All 26 accepted external SWFs
match exact traces in both profiles.
