# binding.field

[Русский](STYX_RPC_LIVE_FIELD.ru.md) · [Live variables](STYX_RPC_LIVE_VARIABLES.md) · [Bridge value types](STYX_RPC_VALUE_TYPES.md) · [Implementation](../src/styxrpc/session_live_values.c#L233)

## Syntax

```actionscript
binding.field(name:String):Object
```


## Parameter and result

Pass one nonempty immediate field name without a slash or leading dot. The binding must be a published record. Returns a new proxy for that field, which may itself be a record. The child owns its own held fid and inherits the parent's write ceiling and the field's permissions. Closing the parent leaves the child usable while the session and child's publication remain available.

## Errors and waiting

Bad argument types fail locally. A missing, unreadable or unsupported field, a non-record binding or a stale publication raises an exception. The synchronous check and walk can stop in `asinc`. A failing call installs no field binding.

## Example

```actionscript
// Retain the field, then return the parent fid immediately.
var details = vfs.liveVariable("/live/.var/details");
var name = details.field("name");
var parentClosed = details.close();
var initial = name.get();
var fieldClosed = name.close();
```

[Publish a native record](../../disVM/runtime/VFS-VARIABLE-RECORD.md) · [Close a proxy](STYX_RPC_LIVE_CLOSE.md)
