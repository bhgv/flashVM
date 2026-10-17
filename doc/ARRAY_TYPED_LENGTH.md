# Typed native Array.length / Типизированная длина Array — v0.527

The original native Array length accessor in loaded SWF4–8 retains the assigned
value, including String, Boolean, null, undefined and Object. Assignment converts
the input once through AVM1 Number/Int32 semantics for truncation, then stores the
original value and primitive metatable. Readonly length still allows conversion
and tail deletion before refusing publication. Successful writable publication
clears property version bits. A conversion error/throw prevents publication;
callback mutations and deletions already performed remain visible.

Truncation reads current storage after conversion callbacks. Only an old Number
controls tail deletion; an old String/Object is never coerced for this purpose.
Array traversal and indexed growth separately coerce the retained live value.
Read-only methods preserve it; native operations that write length normalize it
to their existing numeric result. Typed storage always selects the generic path.
Unloaded embedding assignment retains its existing Uint32/integer validation.

[GC:] The setter roots its input across conversion and errors. Native storage
then traces Object, String and primitive-metatable edges through the existing
collector. Self-cycles and shared graph identity use the ordinary graph map.
Graph transport now accepts all valid value tags in native Array state, with the
same checks for reference bounds, owned strings and forbidden Object metadata.
Malformed tags/references fail before destination object allocation. The public
ABI and wire layout are unchanged; older readers reject newly supported
nonnumeric native Array payloads. Do not send those payloads to older peers.

В загруженных SWF4–8 штатный native-accessor length сохраняет исходное значение,
включая строку, Boolean, null, undefined и объект. Для удаления хвоста отдельно
вычисляется Int32, преобразование выполняется один раз. Старое значение длины
читается после callback; удаление учитывает только старый Number, без повторного
преобразования старой строки/объекта. Readonly запрещает публикацию, но не
предшествующее преобразование и удаление. Ошибки не откатывают callback-изменения.
Обход и рост индексов используют преобразованную живую длину; методы, реально
записывающие length, заменяют её числовым результатом. Незагруженный embedding
сохраняет прежние ограничения присваивания. GC и перенос сохраняют циклы,
строки и метатаблицы; неверные ссылки и типы графа отклоняются до выделения
объектов назначения. Старые читатели графов не поддерживают новые payload.

Tests / проверки:

- `array_typed_length`: SWF4–8, nine nonnumeric inputs, readonly/version flags,
  C API, interpreter, explicit/automatic JIT, primitive metatable cycles, graph
  roundtrips, join preservation, later numeric assignment and indexed growth.
- `array_typed_length_roots`: actual collecting direct setter and normal C API,
  detached input graphs, callback mutation, errors/throws and root unwinding.
- `array_typed_length_allocation`: every assignment/restoration allocation
  boundary, rollback of decoder objects, GC and successful retry.
- `array_typed_length_wire`: self-referential native state and CRC-valid invalid
  object references, Object metadata and value tags.

Reference comparison: Ruffle commit
`2372a4e148a67d68efd796f4477b3cddd4f880cd`,
`core/src/avm1/object/script_object.rs`, `set_local`, `length`, `set_length`.
This is source comparison, not an original Flash Player oracle run.
Deleted/recreated and virtual length assignment semantics require separate work;
complete Array/SWF compatibility and all nine roadmap stages remain open.
Windows/macOS/Android runtime acceptance is not established by Linux tests.
