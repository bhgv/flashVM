> Historical checkpoint. Актуальные изменения v0.13: [EN](STANDARD_OBJECTS.md), [RU](STANDARD_OBJECTS.ru.md).

# Native String checkpoint — v0.12

The opt-in standard library now installs String constructor/prototype metadata.
The existing String(value) function still returns a primitive string; new String
through [`fvm_construct`](../include/flashvm/value.h#L158) or AVM1 NewObject creates a wrapper. No argument produces
an empty string; explicit undefined produces "undefined". The internal string
slot is independent of editable ordinary properties. Input must be valid UTF-8.

Implemented instance methods are valueOf, toString, charAt, charCodeAt, substring,
slice, substr, indexOf, lastIndexOf and concat. The static native method
String.fromCharCode accepts multiple numeric arguments and converts them modulo
65536. The getter-only length accessor is present on each instance and on the
prototype. Ordinary assignment to length leaves its computed value unchanged.
This does not establish full Flash property attributes or enumeration behavior.

## UTF-16 indexing

Length, indexes and ranges count UTF-16 code units, not bytes or Unicode scalars.
A supplementary scalar counts as two units. charCodeAt can return either unit of
a surrogate pair. Substring operations preserve a complete pair when both units
are selected. Search returns code-unit positions. Numeric arguments use the
existing ToNumber bridge followed by integer truncation and bounded clamping;
negative slice/substr starts are relative to the end. substring swaps reversed
bounds. Omitted/undefined range ends use the string length. Empty needles and
out-of-range positions are tested.

The [AS2 String reference](https://open-flash.github.io/mirrors/as2-language-reference/String.html)
is the API reference. The implementation uses UTF-16 indexing consistently;
older-player byte indexing and SWF-version-dependent coercion remain pending.

**Current representation limit:** the C ABI stores NUL-terminated UTF-8 scalar
strings. Embedded NUL and isolated UTF-16 surrogates cannot be represented.
fromCharCode producing either, charAt selecting a single surrogate, or a range
cutting a pair returns [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46) without replacement or truncation.
charCodeAt still reports those units. A full length-aware UTF-16 string model is
required before declaring String complete.

valueOf/toString and length require a String receiver. The other methods can
coerce an ordinary receiver through its native conversion methods. Native
conversion exceptions propagate to AVM1 handlers. Script function-valued methods
and primitive method dispatch are not implemented. Bad argument/receiver results
are C API codes; standard TypeError/RangeError objects remain unfinished.

## Roots, transfer and allocation failure

Constructor capture and prototype links are GC roots. Internal slot storage is
context-owned; strings are not independently collected yet. Removing the global
String binding does not destroy the registered constructor. Native instanceof
recognizes class metadata. Queue and file graph transfer preserve contents,
cycles, prototype identity, methods and the length getter. Separate file writer/
reader processes restore String without destination standard-library installation.
An imported object matches its imported class, not a separately installed class.

The serializer adds kinds 26/27 and callback IDs 501, 502 and 510–519. Duplicate
valueOf/toString callbacks encode with the same identity. Existing envelope
version is unchanged; older readers reject the new kinds. String state rejects
malformed UTF-8 and embedded NUL before restoration. Integrity checks are not
authentication. Import does not register a new global constructor.

Allocation injection covers atomic installation, Error/Number/String creation,
String methods and length lookup. Native method and property-read bridges now
catch allocation failure in lookup/accessor/callback execution and return
[FVM_OUT_OF_MEMORY](../include/flashvm/flashvm.h#L44) with undefined output instead of escaping a C++ exception.

Tests: standard_string.c, standard_string_avm1.cpp, string_edges.c, string_wire.c,
error_allocation.cpp and the extended error_transfer_process.c. See
STRING_CHECKPOINT.md for build counts and sanitizer restrictions. The .as example
illustrates the API; executable tests construct AVM1 records directly.

Still pending: split and Array results, Unicode case mapping, complete Object/
Function integration, primitive method dispatch, versioned semantics, full string
representation and actual Windows/macOS/Android execution. This is a tested
subset, not completion of String or SWF 8.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_construct` | function | [include/flashvm/value.h:158](../include/flashvm/value.h#L158) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
