# FlashVM internal containers (English)

## Scope and compatibility

FlashVM uses internal C++98 types in `src/details/` rather than STL containers:

| Former type | Replacement | Location |
| --- | --- | --- |
| `std::string` | `fvm_detail::string` | `src/details/string.hpp` |
| `std::vector<T>` | `fvm_detail::vector<T>` | `src/details/vector.hpp` |
| `std::map<string,T>` | `fvm_detail::map<fvm_detail::string,T>` | `src/details/hash_table.hpp` |

These are **private implementation types**, not public C ABI types. Embedding applications and extensions use the opaque [`FVM_Context`](../src/core/runtime.hpp#L257), [`FVM_Object`](../src/core/runtime.hpp#L133) and [`FVM_Value`](../include/flashvm/value.h#L26) API in `include/flashvm/`.

### Byte strings

`fvm_detail::string` owns its bytes and always allocates a terminating NUL. It supports construction from NUL-terminated data or a pointer and explicit length; copy, assignment, equality, `size()`, `empty()`, `data()` and `c_str()`. Copies are independent. Binary keys containing NUL are compared using their full length; C API property names remain NUL-terminated.

**Caution:** this is a byte container, **not** AVM1 Unicode string semantics. `c_str()` is valid only while the string is alive and unmodified. Allocation can throw `std::bad_alloc` internally; C ABI entry points must translate exceptions to error codes. The current implementation has no small-string optimization.

### Hash map and vector

The internal hash map uses buckets for lookup and a linked chain for stable insertion-order enumeration; it does not reproduce `std::map`'s sorted ordering. `hash_key(string)` hashes the full byte sequence. Numeric-key maps remain hash-based. The vector is a dynamically resized contiguous array.

### Hybrid table and metatables: current state

[`FVM_Object`](../src/core/runtime.hpp#L133) currently has **two separate containers**: `properties` (string-key hash map) and `indexed` (unsigned-long-key hash map). This is **not yet** a Lua-style unified array/hash/queue table. The desired future representation is a dense numeric array, a sparse hash part, and explicit queue operations with well-defined ordering.

`FVM_Object::metatable` is a shared pointer to another ordinary [`FVM_Object`](../src/core/runtime.hpp#L133), not a copy. Current `__index` and `__newindex` table fallbacks are limited and are **not** a complete Lua metatable implementation. Function-valued metamethods, metatables on primitive values, generalized metamethod dispatch, and Lua-style `setmetatable`/`getmetatable` semantics remain planned. AVM1 prototype lookup is separate from metatable lookup.

### Example and verification

See `tests/detail_string.cpp` for string ownership, binary keys, copying, and map lookups; run `ctest --output-on-failure` after building. See `examples/embedding/tables_debug/` for the supported public C API; do not include private headers in external extensions.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Context` | struct | [src/core/runtime.hpp:257](../src/core/runtime.hpp#L257) |
| `FVM_Object` | struct | [src/core/runtime.hpp:133](../src/core/runtime.hpp#L133) |
| `FVM_Value` | struct | [include/flashvm/value.h:26](../include/flashvm/value.h#L26) |
<!-- source-links:end -->
