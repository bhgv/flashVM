# Native metamethod callbacks (stage 23)

`flashvm/metamethod.h` defines an additive C89 ABI for native callbacks stored on
an ordinary shared metatable. Use [`fvm_table_set_metatable`](../include/flashvm/table.h#L24) to attach the table
and [`fvm_table_get_metatable`](../include/flashvm/table.h#L26) to retrieve the **same pointer** (no copy).

Supported registered names: `__index`, `__newindex`, `__iterate`, `__gc`.
Callbacks receive `(context, self, argc, argv, result, userdata)`. In every
callback `argv[0]` is the owner object and `argv[1]` is the metatable where
the callback was registered. `__index` receives the string key in `argv[2]`;
`__newindex` receives key and new value in `argv[2]` and `argv[3]`.
`__iterate` receives the zero-based call number in `argv[2]`. It returns one
value, or [`FVM_VALUE_UNDEFINED`](../include/flashvm/value.h#L16) to stop. The current API does **not** yet
return a separate key for custom iterators; `out_key` remains undefined.
Without a callback, iteration enumerates indexed and named own properties.

`__gc` receives a third reference to the object being finalized. It runs at
most once when [`fvm_meta_finalize`](../include/flashvm/metamethod.h#L60) is called, or during context destruction.
**This is not a garbage collector**: unreachable objects are not discovered
and collected incrementally. C-only userdata, primitive metatables, AS
function values, `pairs()` syntax and the other Lua metamethods are not yet
implemented. Callback exceptions are caught at the ABI boundary.

## Important distinctions from Lua

The current AVM1 object prototype chain is searched before `__index`. An
existing own property bypasses `__newindex`. Metamethods registered here are
native C callbacks, not values in the metatable's named-property hash. This
will need unification when first-class function values become available.
Callbacks must not retain borrowed strings beyond their lifetime or attempt
to destroy the context while finalizers are running. A callback error during
context destruction cannot be reported by the void destroy API.

See `tests/metamethods.c` for a compiling, executable C89 example.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_meta_finalize` | function | [include/flashvm/metamethod.h:60](../include/flashvm/metamethod.h#L60) |
| `fvm_table_get_metatable` | function | [include/flashvm/table.h:26](../include/flashvm/table.h#L26) |
| `fvm_table_set_metatable` | function | [include/flashvm/table.h:24](../include/flashvm/table.h#L24) |
| `FVM_VALUE_UNDEFINED` | enum constant | [include/flashvm/value.h:16](../include/flashvm/value.h#L16) |
<!-- source-links:end -->
