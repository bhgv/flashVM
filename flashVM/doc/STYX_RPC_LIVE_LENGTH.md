# binding.length

[Русский](STYX_RPC_LIVE_LENGTH.ru.md) · [Live variables](STYX_RPC_LIVE_VARIABLES.md) · [Bridge value types](STYX_RPC_VALUE_TYPES.md) · [Implementation](../src/styxrpc/session_live_values.c#L183)

## Syntax

```actionscript
binding.length():Number
```


## Result

Reads the checked projection's current count as a nonnegative `Number`, up to 4,294,967,295. On an Int32 array or `big` vector this is the element count. On a record it is the number of published named fields. On a hybrid table it is the dense length from the provider's `.len`. The count is a sample and can change before the next operation. The method takes no arguments.

## Errors and waiting

A scalar or string has no collection length. A closed or stale binding, malformed count, I/O failure or stop raises an exception. The metadata read is synchronous and can stop in `asinc`.

## Example

```actionscript
// Sample the collection size before using an index.
var scores = vfs.liveVariable("/live/.var/numbers");
var count = scores.length();
var closed = scores.close();
```

[Read one indexed element](STYX_RPC_LIVE_GET.md)
