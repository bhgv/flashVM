# AS1/AS2 URL functions

v0.619 adds `escape(value)` and `unescape(value)` to [`fvm_standard_install`](../include/flashvm/standard.h#L20).
They are available in host contexts and loaded SWF5–8. A SWF4 invocation returns
[`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L46), including when installation preceded loading. Existing own namespace bindings, including native methods and SWF5/6 case aliases,
reject initial installation without replacement. No network request or OS encoding service is used. [Русский](URL_FUNCTIONS.ru.md).

The spelling rules follow the original
[Adobe AS2 global-function reference](https://open-flash.github.io/mirrors/as2-language-reference/global_functions.html).
It encodes all punctuation, including dot and at-sign. This deliberately differs
from ECMAScript/AS3 escape. Unicode byte encoding was also compared with the
[upstream AVM1 implementation](https://github.com/ruffle-rs/ruffle/blob/147cacd6c081fa33e78b3d57efdde8e9415225a2/core/src/avm1/globals.rs);
that implementation is supporting comparison, not an original-player oracle.
No dependency or upstream source is imported.

Implementation: [`fvm_builtin_escape`](../src/standard/url_encoding.cpp#L137) / [`fvm_builtin_unescape`](../src/standard/url_encoding.cpp#L143); public invocation uses [`fvm_call`](../include/flashvm/value.h#L138) or [`fvm_function_invoke`](../include/flashvm/function.h#L65).

## Accepted profile

| Call | Result |
| --- | --- |
| `escape("a@b.c")` | `a%40b%2Ec` |
| `escape("@*_+-./%")` | `%40%2A%5F%2B%2D%2E%2F%25` |
| `unescape("%2b+%2520")` | `+ %20` (one decoding pass) |
| No argument | undefined, without invoking coercion |
| Explicit undefined | Empty string in loaded SWF5–6; `undefined` spelling in SWF7–8 and host contexts |
| Extra arguments | Ignored |
| Unsupported malformed escape / invalid decoded bytes | [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L46), no partial string |

Escape retains only ASCII letters/digits and emits uppercase `%HH` for every
other byte. Source strings are counted UTF-8/WTF-8; NUL is encoded as `%00`.
UTF-16 surrogate pairs produced by the public constructor use ordinary four-byte
UTF-8; lone surrogates retain their three-byte WTF-8 spelling. Decode accepts
hexadecimal case, replaces literal plus with space, and preserves percent-decoded
plus. It validates the completed counted WTF-8 result before publication.

Malformed `%`, `%0`, `%GG`, `%uXXXX`, overlong, truncated and out-of-range UTF-8
are intentionally unsupported. Historical malformed-input recovery, invalid-byte
replacement, System.useCodepage/local encodings and exhaustive original-player
version comparison remain open. These functions are not general URI-component
or HTML/XML escaping, and do not supply LoadVars/networking.

## Coercion, ownership and errors

Arguments use normal version-aware AVM1 string conversion. Object getters and
`toString` can execute script, mutate globals, collect or raise an exception;
real errors propagate without retry or rolling back their side effects.
Native direct global calls now honor trusted runtime callback entry, retaining
the copied capture and arguments even when a callback replaces its registration.
Ordinary native callbacks still block collection; an enclosing barrier is never
suspended. No new public facility exposes trust to arbitrary host callbacks.

Results belong to the context. In traced-string GC mode, retain them in reachable
VM storage across later collection; local C values alone are not persistent roots.
No partially encoded/decoded output is published on failure. Completed coercion
allocations/side effects may remain; buffer adoption failures release their
unpublished payload. Allocation errors return [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44); invalid lifecycle
returns [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45); normal script exceptions propagate unchanged.

Coercion and conversion share one work budget. Escape charges each input byte;
decode charges each input byte plus each decoded scalar/lone surrogate validation.
[`FVM_ERROR`](../include/flashvm/flashvm.h#L43) reports exhaustion or an observed cooperative worker stop. The work
bound is not a time or byte-allocation deadline. Zero disables the numeric limit,
not periodic cancellation polling. Use the VM owning thread outside GC/teardown.

## Graphs and verification

Existing numeric callback IDs 150–156 and constant IDs 158–160 stay unchanged.
The URL callbacks use distinct IDs 161/162. Extracted Functions survive graph
transport and sender destruction, restoring trusted collecting coercion. Older
receivers reject these new callbacks as unsupported; the graph envelope is unchanged.

Tests: `url_encoding`, `jit_url_encoding`, `url_encoding_graph`,
`url_encoding_allocation` and `global_call_trusted_capture`. They cover counted
Unicode/NUL/lone-surrogate roundtrips, punctuation, versioned undefined/missing
arguments, malformed-profile rejection, lifecycle/work errors, allocation sweeps,
real SWF5–8 interpreter/manual/automatic JIT, collecting/throwing coercion,
old graph ID stability, sender teardown and self-replaced native captures.
[Annotated C89 tutorial](../examples/embedding/url_encoding/README.md).
These are selected Linux regressions, not complete SWF or foreign-platform acceptance.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_builtin_escape` | function | [src/standard/url_encoding.cpp:137](../src/standard/url_encoding.cpp#L137) |
| `fvm_builtin_unescape` | function | [src/standard/url_encoding.cpp:143](../src/standard/url_encoding.cpp#L143) |
| `fvm_call` | function | [include/flashvm/value.h:138](../include/flashvm/value.h#L138) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `fvm_function_invoke` | function | [include/flashvm/function.h:65](../include/flashvm/function.h#L65) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:20](../include/flashvm/standard.h#L20) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
