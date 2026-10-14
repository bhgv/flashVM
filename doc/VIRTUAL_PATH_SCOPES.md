# Virtual path scopes / Неявные свойства в областях видимости — v0.274

Variable assignment now recognizes intrinsic MovieClip paths when selecting a
With receiver. The assignment creates an own binding on the wrapped object,
including an ordinary object inheriting a concrete MovieClip, instead of writing
an outer activation/target. Scope deletion stops at an intrinsic field and returns
false for its absent own slot, preserving same-named outer bindings.

DefineLocal2's initial has-property check includes virtual paths. Initialized
DefineLocal and named-function placement continue using own-property presence
when deciding whether a With object receives a local declaration. Virtual presence
therefore does not become own presence or alter initialized declaration rules.
SWF4 root/parent and pre-SWF6 global remain ordinary names under the existing
version policy; unloaded host descriptors retain their previous behavior.

The common private path classifier is allocation-free and invokes no getters or
scene-provenance reads. Reading a value uses the same classification followed by
the existing validated scene lookup. Documented path-kind macros are semantic
identifiers, independent of actions and serialized callback IDs.

`execution_virtual_path_scopes` covers C89 SWF4–8 root/parent/global behavior on
direct clips and ordinary objects inheriting clips, assignment owners, deletion
barriers, undefined declarations and initialized declaration placement.
`avm1_virtual_path_scopes` exercises actual With assignment and Delete2 dispatch.
`execution_virtual_path_scopes_allocation` checks all four operation modes at
allocation boundaries, root restoration and preservation of outer bindings.

Внутри With наличие неявного свойства учитывается при присваивании и удалении;
чтение getter для выбора владельца не требуется. Delete2 не удаляет одноимённую
внешнюю переменную, когда внутреннее свойство существует, но не является своим.
DefineLocal2 учитывает такое наличие; обычный DefineLocal по-прежнему проверяет
собственные поля. Полные правила удалённых клипов, свойств, this и стадии роудмапа
остаются открытыми.

Primary reference: pinned Ruffle
[`scope.rs`](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/scope.rs)
`set`, `delete` and `define_local`, plus
[`script_object.rs`](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/object/script_object.rs)
has-property versus has-own-property behavior.
