# Prototype API (implemented subset)

The public C89 header `flashvm/value.h` exports:

- [`fvm_object_set_prototype`](../include/flashvm/value.h#L66): link or clear an object's prototype; reject cycles
  and references across contexts.
- [`fvm_object_get_prototype`](../include/flashvm/value.h#L68): return the borrowed prototype pointer, or NULL.
- [`fvm_object_has_own`](../include/flashvm/value.h#L70): distinguish own properties from inherited properties.
- [`fvm_object_get`](../include/flashvm/value.h#L72): search own properties first, then follow prototypes.
- [`fvm_object_set`](../include/flashvm/value.h#L74): always write an own property (shadow inherited properties).

Objects are context-owned until context destruction. The API is not thread-safe.
Prototype support does not yet implement `__proto__`, `__resolve`, property
attributes, accessor methods, AVM1 constructor behavior, or script-defined
functions. C89 host code can use these functions now; AVM1 `GetMember` uses the
same inherited lookup.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_object_get` | function | [include/flashvm/value.h:72](../include/flashvm/value.h#L72) |
| `fvm_object_get_prototype` | function | [include/flashvm/value.h:68](../include/flashvm/value.h#L68) |
| `fvm_object_has_own` | function | [include/flashvm/value.h:70](../include/flashvm/value.h#L70) |
| `fvm_object_set` | function | [include/flashvm/value.h:74](../include/flashvm/value.h#L74) |
| `fvm_object_set_prototype` | function | [include/flashvm/value.h:66](../include/flashvm/value.h#L66) |
<!-- source-links:end -->
