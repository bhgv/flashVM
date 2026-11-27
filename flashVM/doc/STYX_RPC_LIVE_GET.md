# binding.get

[Русский](STYX_RPC_LIVE_GET.ru.md) · [Live variables](STYX_RPC_LIVE_VARIABLES.md) · [Bridge value types](STYX_RPC_VALUE_TYPES.md) · [Implementation](../src/styxrpc/session_live_values.c#L84)

## Syntax

```actionscript
binding.get([index]):Object
```


## Parameters and result

Omit `index` for a scalar Int32, a scalar `big` or a string. For an Int32 array, pass a nonnegative integral `Number` below the live length. For a `big` vector, pass a signed index. `-1` selects the last element. The numeric half of a hybrid table takes an exact signed key and resolves negative indices relative to the dense portion. A `big` key or position accepts the same safe `Number` or decimal `String` forms as a `big` value.

Returns the current `Number`, a canonical decimal `String` for a wide `big`, a counted Unicode string, or `null` for a nil string. After return, the value belongs to the movie. A later call reads the owner's current value.

## Errors and waiting

A wrong index type or arity is an argument error. An invalid or absent element, closed or stale binding, malformed reply or failed I/O raises an exception. Use [`field`](STYX_RPC_LIVE_FIELD.md) first for a record. The call is synchronous and observes stop in an `asinc` task. A stopped read keeps the binding available while its publication still matches.

## Example

```actionscript
// Every call samples the owner's vector.
var vector = vfs.liveVariable("/live/.var/details/wide");
var last = vector.get(-1);
var closed = vector.close();
```
