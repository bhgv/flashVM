# Error and native string conversion — v0.9

[`fvm_standard_install`](../include/flashvm/standard.h#L22) now atomically installs the AS2 Error constructor metadata
alongside Math and Boolean. `fvm_construct(..., "Error", ...)` and AVM1 NewObject
create an Error with an ordinary, editable prototype. Error.prototype.name and
message default to `"Error"`. Omitted or undefined arguments inherit that message;
other arguments are retained as values. Extra arguments are ignored.

Error.toString converts the message to a string, without prepending the name.
This follows the [AS2 Error reference](https://open-flash.github.io/mirrors/as2-language-reference/Error.html).
Changing name therefore does not change the default string result. The method
can be borrowed by another object. A native message getter or conversion method
can throw a pending exception, which AVM1 try/catch handles.

## Native APIs and AVM1

The C89 API in `flashvm/standard.h` provides [`fvm_value_to_string`](../include/flashvm/standard.h#L36) and
[`fvm_value_instance_of`](../include/flashvm/standard.h#L42). The former copies string arguments into context-owned
storage, supports in-place conversion, formats primitives and invokes native
object toString/valueOf methods. Conversion recursion is limited to 64 calls.
Finite numeric formatting searches up to 17 significant digits for a libc
round trip and normalizes the decimal separator and exponent spelling. Tests
cover boundary values and 256 deterministic binary64 samples; this is not a
proof of identical Flash formatting for every number or SWF version. Do not race
host locale changes during conversion.

The instanceof bridge and AVM1 opcode 0x54 compare prototype identity for the
native Boolean, Error, TextFormat and TextField class metadata. Ordinary objects
with a prototype property are not accepted as constructors. Primitive operands
return false; malformed constructors are rejected. Prototype walks are bounded.

Resource exceptions retain their code, message and context. If Error is installed,
materialization attaches its current registered prototype. An own native toString
method is supplied even when the standard library has not been installed.
Native resource calls retain their C result-code contract; ActionScript calls
produce pending exceptions as described in [EXCEPTIONS.md](EXCEPTIONS.md).

## Collection and channel transfer

Constructor capture, instance prototypes, message object values, method roots and
pending exceptions are traced by the collector. Error graphs use the existing
channel graph serializer, including cycles, aliases, prototypes and callback
301 for the built-in Error.toString. Queue and file restoration preserve methods
without installing the standard library in the destination. File writer and
reader are tested as separate processes. Resource exception snapshots contain
ordinary diagnostic data, not an implicit live copy of a closed resource.

The receiver gets a new graph identity. An imported instance matches its imported
constructor, not a separately installed destination Error constructor. Sending
the constructor does not register a new global constructor in the receiver.
Readers predating v0.9 do not understand the new Error kinds and callback ID.

## Examples, tests and limits

See [examples/actionscript/error.as](../examples/actionscript/error.as),
`tests/standard_error.c`, `tests/actionscript_error.cpp`,
`tests/string_conversion.c`, `tests/error_allocation.cpp`, and
`tests/error_transfer_process.c`. The independent-process driver is
`scripts/test_error_process.py`. Allocation injection verifies failed installation
publishes no partial globals, failed construction clears its output, and failed
exception materialization retains the raw fault for a later retry after GC.

This is a native Error checkpoint, not the complete Flash object model. Error is
not yet a first-class Function, calling Error without new is unsupported, and
script function-valued conversion methods are not dispatched by this bridge.
Object/Function constructors, version-dependent attributes/coercion, Function2,
full text/string semantics, complete standard objects, JIT and screen adapters
remain unfinished. The .as example documents the API; there is no AS source
compiler in this repository. AVM1 tests construct executable action records.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_construct` | function | [include/flashvm/value.h:232](../include/flashvm/value.h#L232) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:22](../include/flashvm/standard.h#L22) |
| `fvm_value_instance_of` | function | [include/flashvm/standard.h:42](../include/flashvm/standard.h#L42) |
| `fvm_value_to_string` | function | [include/flashvm/standard.h:36](../include/flashvm/standard.h#L36) |
<!-- source-links:end -->
