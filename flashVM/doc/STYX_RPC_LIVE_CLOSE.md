# binding.close

[Русский](STYX_RPC_LIVE_CLOSE.ru.md) · [Live variables](STYX_RPC_LIVE_VARIABLES.md) · [Bridge value types](STYX_RPC_VALUE_TYPES.md) · [Implementation](../src/styxrpc/session_live_values.c#L208)

## Syntax

```actionscript
binding.close():Boolean
```


## Result and effects

Takes no arguments. Releases the owned fid and returns `true` the first time. A later call returns `false`. All aliases of that proxy see the closed state. An independently acquired field has its own fid. Closure is also valid after the peer removes the path. Return values describe local ownership. A collector can queue deferred cleanup when the last alias becomes unreachable.

Explicit closure runs on the owner and can wait for ordinary fid cleanup. Perform it outside an overlapping operation on that session. A wrong number of arguments fails before closing.

## Example

```actionscript
// Explicitly relinquish a temporary read binding.
var name = vfs.liveVariable("/live/.var/details/name");
var currentName = name.get();
var firstClose = name.close(); // true
var secondClose = name.close(); // false
```

[Open a binding](STYX_RPC_LIVE_VARIABLE.md)
