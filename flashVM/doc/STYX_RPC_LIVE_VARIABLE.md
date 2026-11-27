# vfs.liveVariable

[Русский](STYX_RPC_LIVE_VARIABLE.ru.md) · [Live variables](STYX_RPC_LIVE_VARIABLES.md) · [Bridge value types](STYX_RPC_VALUE_TYPES.md) · [Implementation](../src/styxrpc/session_live_values.c#L310)

## Syntax

```actionscript
vfs.liveVariable(path:String [, writable:Boolean]):Object
```


## Parameters

| Name | Meaning |
| --- | --- |
| `path` | Path in the mounted session. One leading slash is optional. Use named components and the published `.var` and `.prog` components where present. |
| `writable` | Boolean access ceiling. Omit it or pass `false` for a read-only binding. Pass `true` to request writes within the permissions of the publication. A collection's read-only metadata remains readable. |

## Returns

An owner-local object with `get`, `set`, `length`, `field` and `close`. The methods available for a particular node depend on its advertised type. The object owns one unopened fid and captures the published Qid path. Each value operation checks the path and addresses the original fid. Keep the supplying session open while operating on the proxy. Close the proxy as soon as it is no longer needed.

## Errors and availability

Available on the movie's mounted `vfs` session and a mounted `asinc` worker session in a hosted Styx RPC build. A malformed path or non-Boolean access flag is an argument error. An unknown projection, missing publication or unavailable access mode raises an exception. This synchronous walk and type check can stop in an `asinc` task. No proxy is exposed if admission fails.

## Example

```actionscript
// Capture the owner's exact Int32 array with read access.
var scores = vfs.liveVariable("/live/.var/numbers");
var firstScore = scores.get(0);
var closed = scores.close();
```

[Read a value](STYX_RPC_LIVE_GET.md) · [Release a binding](STYX_RPC_LIVE_CLOSE.md)
