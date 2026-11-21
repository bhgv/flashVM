# styxunibusserverloadprofile

[Русский](UNIBUS-SERVER-LOAD-PROFILE.ru.md) · [Header](unibus_server.h) · [Implementation](unibus_loaded.c)

```c
DVM_Provider *styxunibusserverloadprofile(StyxUnibusServer *server);
```

Include `unibus_server.h` and link `styx_unibus_server`. Pass the live
server returned by [styxunibusserverstartloaded](UNIBUS-SERVER-LOADED.md).
The call returns immediately with a provider borrowed until join. It
returns NULL for other server modes or a NULL server.

Grant only write access to the provider's root node in the owner's VFS.
Pass the grant path as the `offer` field of the `loaded` configuration.
The owner writes one complete S-record after resolving the export. The
dispatcher verifies the selected name and signature and seals the
mailbox when it reads that record. Remote clients read the accepted
profile from `.interface` on the mounted directory.

## Compiled types

If the loaded image contains a [compiled type table](../../../tools/limbo-bootstrap/STYX-TYPE-TABLE.md),
the service includes the selected export's types in the offer. S-record
tag 88 carries the existing interface kind and 32-bit signature, one-byte
lengths for name, arguments and result, then those three counted strings.
The two type descriptions are outer lists. Inside them `i`, `b`, `f`, and `y`
stand for `int`, `big`, `real`, and `byte`, parentheses nest a tuple, and `r[ib]`
describes a reference to an ADT with those two fields.

The owner validates the schema against the supported gateway, and client
`load` verifies the schema and its compiled name and signature before
binding. The three-argument gateway uses the received lists to encode the
call and decode the result. Images without a type table continue to use
tag 71, which carries the compiled name and signature for the six fixed
profiles. Additional [compiled scalar shapes](SCALAR-FUNCTION.md#calls-with-compiled-scalar-types)
and [reference layouts](SCALAR-FUNCTION.md#calls-with-compiled-reference-types)
require type metadata from both images.


## Value records for gateway development

The [codec](call_record.h) validates an entire typed list before exposing a
borrowed view. Tag 7 contains counted UTF-8 bytes, including embedded
zeroes. Tag 8 contains nested records of the element type declared by
`a[T]`. Tag 9 with an empty payload represents `nil` in a string or array
position. Empty strings and arrays retain their own tags. Tuple and scalar
records use their existing nested tags. The complete list record is limited
to 65,536 bytes.

The NC64 layout calculation identifies field offsets and roots for strings,
arrays, and references. Styx invocation still uses the existing profiles.
The new path still needs object allocation in the receiving VM, integration
with the root map, and reference copy-back.
