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
The two type descriptions are outer lists. Inside them `i` and `b`
stand for `int` and `big`, parentheses nest a tuple, and `r[ib]`
describes a reference to an ADT with those two fields.

The owner validates the schema against the supported gateway, and client
`load` verifies the schema and its compiled name and signature before
binding. The three-argument gateway uses the received lists to encode the
call and decode the result. Images without a type table continue to use
tag 71, which carries the compiled name and signature. The supported
runtime signatures are the six established gateway profiles.
