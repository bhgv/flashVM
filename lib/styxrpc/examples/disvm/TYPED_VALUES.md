# Pass strings, arrays and exact big values between FlashVM and DisVM

[Русский](TYPED_VALUES.ru.md) · [SDK examples](../README.md) · [FlashVM type reference](https://github.com/bhgv/flashVM/blob/master/flashVM/doc/STYX_RPC_VALUE_TYPES.md)

This paired example loads a Limbo library from the live VFS, calls three typed exports and checks the results in an AS2 movie. It exercises independent copies, one-call reference identity and a channel kept live after the method returns.

## Build and run

From the FlashVM repository root, use the native DisVM, Limbo bootstrap,
paired FlashVM player built with the installed libstyxrpc, and AS2 tools.
The paths below assume the workspace build directories shown in the
[FlashVM build guide](https://github.com/bhgv/flashVM/blob/master/flashVM/doc/STYX_RPC.md#build).
Substitute the paths to your binaries.

```sh
python3 lib/styxrpc/tests/flashvm/mixed_pair.py \
    --source disVM --runtime build/disvm/disvm \
    --limbo build/disvm/bootstrap/limbo-bootstrap \
    --player build/flash/flashvm_styx_player \
    --as2 out/as2/as2_viewc --fir out/fir/fir1 \
    --fir-opt out/fir/fir1_opt --swl out/linker/swl --live-values
```

The launcher compiles the library and the movie, opens the paired session and enables `--live-dis-load` on its DisVM mount. It runs in the interpreter over a Unix socket by default. Add `--jit` for the JIT and `--tcp` for loopback TCP. Both flags can be used together.

```text
PASS native live-values RPC paired VMs
PASS Dis application completed
```

The first line is printed by the Limbo application after it sees `/wide-done`. A rejected method, failed value check or missed completion deadline prevents that marker and causes the launcher to fail. The launcher removes the temporary workspace after the pair exits.

## Follow the exchange

1. The Limbo [library](../../tests/flashvm/live-values_library.b) declares `wide`, `patch` and `widepipe`. DisVM uses the compiled exports to derive the signatures and shapes. The [Limbo application](../../tests/flashvm/live-values_pair.b) uploads the [AS2 movie](../../tests/flashvm/live-values_pair.as); the movie calls `vfs.open("/wide.dis").load()` and closes the source file. The held proxy stays usable. [Loaded files](https://github.com/bhgv/flashVM/blob/master/flashVM/doc/STYX_RPC_LIVE_DIS.md)
2. The movie's [call sequence](../../tests/flashvm/live-values_calls.as) passes both an `array of string` and an `array of big` to `wide`. The first slot of each carries digits. Only the slot typed `big` is decoded as binary signed 64-bit. Dis changes the second string to `"copy"`, subtracts one from the largest `big`, and returns both arrays in a tuple. AS observes both edits in the result and still sees its original input arrays. [Managed calls](../../reference/managed_invoke.md)
3. `patch` checks the minimum `big` in a one-field `ref Wide`, changes the field to the maximum `big` and returns the same ref. AS sees the change in the original Array after the call. Assigning through the returned Array changes that same local object. [Graph calls](../../reference/graph_invoke.md)
4. `widepipe` receives an array from the lent queue and puts its edited contents on a newly created result channel. The caller holds that channel while receiving its single element. An empty buffered input queue would leave the Dis receive waiting, so the movie enqueues before the call. It closes both channels and releases the module with `os.unload(proxy)` before publishing the completion marker. [Channel lease](../../reference/channel_leases.md)

Here is the send/receive sequence from the AS side, with the same signatures as the runnable example.

```actionscript
var queue = vfs.localChannel("a[b]", 1); // One slot holding an array of big.
var values = "".split("");
values.push("9223372036854775807", "-9223372036854775808");
if (queue.trySend(values)) {
    var reply = proxy.widepipe(queue); // Lend the live queue for this call.
    var edited = reply.recv();        // ["9223372036854775806", "-9223372036854775807"]
    reply.close();                   // Release the distinct result lease.
}
queue.close();
```

The numeric strings in the example denote exact values. A safe integer such as `42` can be sent as an AS `Number` and decodes back to `Number`. A named untyped data message preserves decimal text. [AS mappings](https://github.com/bhgv/flashVM/blob/master/flashVM/doc/STYX_RPC_VALUE_TYPES.md)

## Check the same integer record with the C SDK

The C SDK accepts an `int64_t` directly. The function below exits with status
zero if an encoded minimum `big` decodes exactly. Compile it as C99 and link
`StyxRpc::rpc` through `find_package(StyxRpc CONFIG REQUIRED)`.

```c
#include <styxrpc/managed.h>

int main(void)
{
    unsigned char bytes[64];
    size_t used;
    int64_t result;
    if (styxrpc_value_integer(STYXRPC_VALUE_BIG, INT64_MIN, bytes,
            sizeof(bytes), &used) != STYXRPC_OK)
        return 1; /* No complete child record was produced. */
    if (styxrpc_value_get_integer(STYXRPC_VALUE_BIG, bytes, used, &result) != STYXRPC_OK)
        return 1; /* The signed big record did not validate. */
    return result == INT64_MIN ? 0 : 1;
}
```

[Encode a child record](../../reference/value_integer.md) · [Decode a child record](../../reference/value_get_integer.md)
