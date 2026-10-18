# Nested native GC root scopes

Native extensions can open multiple temporary root scopes. Scopes form a strict last-in, first-out stack. Closing an outer scope while an inner scope remains open returns [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45) and leaves both scopes intact.

Each scope copies [`FVM_Value`](../include/flashvm/value.h#L24) entries. Object entries protect the referenced objects until that scope is closed; this does not transfer ownership of the objects or make their raw pointers safe after collection.

The regression test `tests/gc_nested_scopes.c` checks nesting, invalid closure order, survival through collection, and reclamation after scope closure. This is **not** a substitute for full AVM1 and native-call root integration, which remains incomplete.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_Value` | struct | [include/flashvm/value.h:24](../include/flashvm/value.h#L24) |
<!-- source-links:end -->
