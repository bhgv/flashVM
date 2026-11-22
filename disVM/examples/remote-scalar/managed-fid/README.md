# Load a managed Dis library over Styx

[Русская версия](README.ru.md) · [Remote Dis examples](../README.md)

Two DisVM processes share one Styx session. The owner serves four exports from
one loaded instance of [target.b](target.b). The caller imports [managed.m](managed.m),
loads the mounted module path and calls the functions with ordinary Limbo syntax.
The owner builds the bridge from the types recorded by the Limbo compiler.

## Run

Build the native Styx SDK and the DisVM runtime with the same SDK. From the
repository root, give the runner the matching DisVM and Limbo compiler binaries.

```sh
python3 disVM/examples/remote-scalar/managed-fid/run.py \
    build/scalar-runtime/disvm build/scalar-runtime/bootstrap/limbo-bootstrap
```

The runner uses separate temporary directories, compiles the images with
`-D RG`, connects both processes over a single loopback TCP connection and
cleans up on exit. Expect `PASS managed Styx load` in both execution modes.

## Follow the call

1. The client's `load` matches the owner signature and schema in `.interface`
   against its compiled import. The functions are files inside `managed.dis`.
2. The client opens one function fid and writes a complete S-record with
   `(client PID, arguments…)`. The record can span several Styx writes. Its
   encoded length tells the owner when to schedule the call.
3. The owner gives the request to the caller's worker. The generated bridge
   decodes a local copy of the arguments and calls the original export with
   `mcall`. It writes the result through the private route for that fid.
4. The client reads and closes the fid. The reply has positions for the result
   and reference updates. The result is `nil` for a void export.

The example checks a UTF-8 string, a returned byte array, a void return, an
owner exception and two parallel calls whose records exceed one Styx message.
It also passes the same ADT through two `ref` arguments and verifies that an
unchanged ADT retains its original array on the client. It checks array identity
again after changing only the string in the very same ADT.

For `ref` arguments, the request defines each non-nil ADT once. Later aliases
carry its index. The reply's second position carries a bit mask of changed
references in flattened argument order. On an owner that publishes the
`.ref-fields` capability, the client opts in on each request and each selected
reference has its own field mask and only the selected values. Tuples are
flattened in schema order. An empty position means no copy-back. The owner compares final field
values against the request, so a store of the same value does not set a bit.
The client validates the full response before applying the return value and
updates together. An exception discards all copy-back.

The profile supports bounded ADTs with scalar, string, array and
tuple fields. Nested `ref` fields and `ref` return values are outside this
profile. Arrays are individual fields. A two-byte `.ref-fields` file advertises multiple
spans. For equal-size encoded arrays the owner sends sorted offset-and-length
runs when they cost less than one span or a full array. Short gaps are merged
when that saves bytes. The client checks the bounds and ordering of every run
and validates the reconstructed array against its type before publishing it.
A three-byte `.ref-fields` file also advertises splices for size changes.
The owner sends the offset, the number of bytes to remove and the insertion
bytes when that representation is smaller. The common prefix and suffix
are reused from the request. Wide edits can still use the complete array.
Peers without the capability still use the complete object snapshot
for each changed reference.

A four-byte `.ref-fields` file also advertises patches for UTF-8 strings.
They can cross character boundaries. The receiver validates the complete
reconstructed string before applying the update. Older peers receive the
complete string field.

A five-byte `.ref-fields` file advertises multiple replacements in one value.
Each replacement uses coordinates in the original value. The owner searches
a bounded neighborhood for a shared sequence after each mismatch. If the
resulting record saves bytes, it can preserve a long interior between an
insertion and a distant replacement. Otherwise the earlier encodings apply.

The nested example sends an array of UTF-8 strings and a `ref` object with
an array of byte arrays and an array of tuples. Each array element has its
own S-record. A nil element and an empty array have distinct records.
The reply returns arrays by value and updates the existing `ref` object.
Array element types may recursively contain scalars, strings, arrays and
tuples within the compiled schema and record limits. References inside
array elements are outside this value format.
