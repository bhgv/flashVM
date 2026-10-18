# Global binding lookup / Поиск `_global` — v0.269

Loaded plain-name resolution now searches ordinary lexical, target and global
bindings before applying the `_global` fallback. An existing undefined property
still counts as present and prevents fallback. The implicit namespace exists in
SWF6–8, with case folding in SWF6 and exact spelling in SWF7–8. SWF4–5 may store
and read ordinary properties with that name, but do not receive an implicit
namespace value. Unloaded host calls retain their exact reserved-name priority
after lexical scopes, preserving the previous host contract.

The existing counted Flash UCS2 keyword matcher is shared by this and namespace
recognition. Documented name/length macros replace embedded keyword identities;
the existing this spelling/version behavior remains covered by its fixtures.
No public ABI or bytecode dependency is added.

`execution_global_binding` covers C89 callers, unloaded/SWF4–8, namespace
availability, case spelling, lexical/target/global shadows, undefined presence,
deletion and embedded NUL. `avm1_global_binding` exercises actual GetVariable
against a concrete MovieClip target. The existing C89 lookup fixture now expects
no implicit `_global` in SWF4–5, and allocator/restoration fixtures remain active.

Qualified path aliases and Delete2 prefix fallback still have their separate old
rules and remain follow-up work. The failing original swf4_vars probe also has
unimplemented parent/system-property behavior; this change does not certify that
SWF or add it to the passing external corpus.

В загруженных SWF обычная привязка `_global`, в том числе undefined, теперь
находится раньше неявного пространства имён. Неявное значение появляется только
с SWF6; SWF6 не различает регистр, SWF7–8 различают. Старый приоритет имени для
незагруженного host-режима сохранён. Правила составных путей и Delete2 уточняются
отдельно; все стадии роудмапа остаются открытыми.

Primary reference: pinned Ruffle
[`scope.rs`](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/scope.rs)
`resolve`,
[`stage_object.rs`](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/object/stage_object.rs)
`resolve_path_property`, and
[`script_object.rs`](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/object/script_object.rs)
ordinary stored-property precedence.

Validation: Linux GCC13.3 AGG/Cairo each pass 515 tests. Four AGG executables
required execute-bit restoration and a successful rerun. Seven selected
ASan/UBSan fixtures pass with heap/global coverage, without stack/leak checks.
All 26 accepted external SWFs match exactly in both profiles. swf4_vars remains
failing in both profiles and is recorded separately, without oracle/header edits.
