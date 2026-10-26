# Copied values and references, derived from Limbo types

[Live document projections](LIVE-PROJECTIONS.md)

[Русский](VALUES.ru.md) · [Projection inspector](PROJECTIONS.md) ·
[Interface](bridge/idevalue.m) · [Executable checks](tests/values_test.b) ·
[AS library](../as/projection-values.as)

## Conversion targets

`Idevalue.types()` returns fresh descriptors from the compiled Limbo module.
Each declares the client type name, Limbo representation, width, optional helper
and copy/reference mode. IDE completion consumes this catalog; clients must use
the target profile, not infer integer width from the browser or host architecture.

| Optional AS helper | Requested type | Limbo representation | Transfer/result |
| --- | --- | --- | --- |
| `tobyte` | byte | byte | Checked 0..255 copy |
| `toint` | int | int | Signed copy; classic 32 or experimental NC64 64 bits |
| `tobig` | big | big | Exact signed 64-bit copy |
| `toreal`, `tofloat` | real | real | Binary64 copy; both names mean the same type |
| `tostring` | string | string | Copied Unicode text |
| `tobool` | bool extension | int | Canonical 0/1 value |
| `tochar` | char extension | int | One Unicode scalar, excluding surrogates |
| `totable` | table extension | named snapshot | Copied fields; nested objects retain references |

Limbo has no independent bool/char/table scalar types. These extensions are
explicit in reflection. This library does not replace Limbo's native conversion
operators or AS Number/String/Boolean semantics. In particular `tofloat` is not
binary32, and `tochar` is not an AS UTF-16 code unit.

`array`, `list`, `chan`, `ref ADT`, modules and functions are not scalar casts.
Future `toarray`/`tolist` require a separately specified snapshot policy;
`tobytes` requires explicit encoding. Live channels and module/device/object
capabilities remain references. A Limbo `ref` keeps its existing language meaning;
no new `ref` source syntax is introduced here. Native source parsers remain
Coco/R based; these modules are compiled using the matching bootstrap until our
FIR frontend can lower their general ADTs/channels.

## Value and reference rules

A scalar constant can be sent as a typed Value, without exporting a variable.
A variable can similarly supply a copied snapshot, or expose a projection when
later reads/writes must reach its owner. `copy` validates the tag, bounds and
payload and creates fresh records/arrays. Changing the sender's or renderer's
record after acknowledgement cannot change the owner/model's state.

An Object descriptor carries logical receiver identity and incarnation. A Method
adds the exact method name **and keeps that receiver identity**. It never carries
a native pointer or serialized code. `InvokeRef` validates receiver/incarnation,
requires explicit consent and executes at that owner. A method from a different
receiver is rejected even when both owners have the same incarnation number.
`startnamed` requires the adapter to supply a unique identity within its session;
identity strings alone are not authentication or a capability registry.

A `ToTable` request is an explicit data snapshot, not an object clone. The sample
owner includes stored number/text fields and a self-reference, omits methods and
does not run getters. `copy(Table)` supports at most 32 ordered unique names and
scalar/reference entries, not recursively nested tables. It rejects nested and
cyclic tables without recursive traversal. Each text/name is bounded to 4096
Unicode characters; aggregate table text/identity budget is 16384 characters.
Objects remain references; a caller cannot silently obtain executable object
state by requesting a scalar conversion. Undefined and Null have separate tags.

## Conversion and error policy

Int/Big conversions accept optional-sign complete decimal strings with signed
range checks, never routing Big through binary64. Whitespace, trailing data,
fractional and overflow inputs are rejected. Real-to-integer conversion admits
only integral values within ±(2^53−1), conservatively avoiding recovery of an
integer that may already have lost precision in a Number. Int admission uses the
compiled target profile; Big remains 64-bit in either profile.

Real text conversion accepts a complete signed decimal fraction/exponent up to
128 characters and rejects malformed or nonfinite results. Finite underflow to
signed zero is allowed. Explicit Big-to-real may round, e.g. 9007199254740993
becomes 9007199254740992. Existing Real values retain their IEEE payload through
copy; their diagnostic string formatting follows the selected VM and is not a
claim of exact Flash Number.toString formatting.

Char accepts a valid integer Unicode scalar or a one-character Limbo string.
Byte rejects values outside 0..255. Bool uses numeric nonzero, nonempty string,
and false for Null/Undefined; the nonempty string `"false"` is true. Object
coercion is not evaluated by this pure helper. Unsupported/invalid conversion
returns nil; the owner publishes Invalid without mutation or a partial value.

## IDE and channel integration

`ReadAs` reads and converts one field at its owner. `Convert` consumes an explicit
copied value, including constants. `WriteValue` validates an exact storage-matching
Int/Text value before assignment; it never stores the caller's mutable wrapper.
`ToTable` and `InvokeRef` have the distinct semantics above. All target mutation
is serialized by the owner. Paused owners reject writes/invocation and allow
copied reads. These are cooperative pause semantics, not arbitrary heap access
under system debugger suspension.

The inspector model validates and copies typed replies before admission and
again when supplying a renderer snapshot. Its existing ticket/epoch/revision
checks still reject stale/duplicate refreshes. The console IDE lists the optional
`Projection.to*` declarations and call tips from the reflection catalog:

```text
complete as Projection.to
hint as Projection.tofloat 1
```

All projection clients must rebuild against the changed `.m` signatures. Classic
and NC64 `.dis` modules are never interchangeable. The package ships matching
modules, headers, source and a fully commented inspector example.

## AS adapter boundary and tested scope

Load `projection-values.as` explicitly and call `makeProjectionValues(request)`.
The supplied adapter function accepts `(operation, reference, type, completion)`;
its ticket/return value is preserved. The helper never reads the opaque receiver,
evaluates a getter or converts a Big to AS Number. `tofloat` is the identical
function as `toreal`. The adapter must schedule work, retain pending roots, validate
replies, preserve wide integers and invoke completion exactly once.

This is an optional library contract, **not** new AS compiler syntax, native
globals or a network serialization implementation. AS syntax is checked with our
Coco/R frontend; the client dispatch contract is tested under Node. Real Dis
channel tests cover constants, typed writes, exact Big, binary64, Unicode,
snapshots, bound receivers and model copy isolation in interpreter/AMD64 JIT.
A separate classic-profile build checks compilation only. Network RPC, arbitrary
FlashVM reflection, exported-object GC and arbitrary debuggee-local inspection remain open.
The live document panel is implemented separately as described above.
