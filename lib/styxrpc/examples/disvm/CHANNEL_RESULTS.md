# Return live channels with copied values and graph changes

[Русский](CHANNEL_RESULTS.ru.md) · [SDK examples](../README.md) · [Provider contract](../../reference/channel_module_init.md)

The native Limbo client loads the same compiled interface from two providers. One is a source-built AS2 movie in the stock FlashVM player. The other is a C program linked to the independently built hosted SDK. Both produce a tuple with a ref alias and two queues, a duplex string endpoint, and a duplex rendezvous for int.

## Build and run

Run from the repository root. The paths below assume a built native DisVM and Limbo bootstrap, the paired FlashVM player and the AS2, FIR and SWL tools. For C, configure [the hosted SDK](../../README.md#build) with `STYXRPC_EXAMPLES=ON` and build `styxrpc-channel-results-device`.

```sh
python3 lib/styxrpc/tests/flashvm/mixed_pair.py \
    --source disVM --runtime build/disvm/disvm \
    --limbo build/disvm/bootstrap/limbo-bootstrap \
    --player build/flash/flashvm_styx_player \
    --as2 out/as2/as2_viewc --fir out/fir/fir1 \
    --fir-opt out/fir/fir1_opt --swl out/linker/swl --result-channels
```

```sh
python3 lib/styxrpc/examples/disvm/channel_results.py \
    --source disVM --runtime build/disvm/disvm \
    --limbo build/disvm/bootstrap/limbo-bootstrap \
    --device build/rpc/styxrpc-channel-results-device
```

Add `--jit` to exercise the JIT. Add `--tcp` for loopback TCP. With neither option the test uses the interpreter and a Unix socket. Both modes print `PASS native result-channels RPC paired VMs`. The C runner also requires `PASS independent C result provider` from the device. The run fails if a required exchange, retirement or process exit cannot be confirmed. Both runners close the carrier and remove their temporary build directory.

## Follow the compiled call

1. [The Limbo contract](../../tests/flashvm/result-channels_pair.b) declares `compose(box, option, absent)` with input shape `(r[i]is)` and result shape `((r[i]a[c[i]]c[s]))`. `absent` is a nil string, decoded as `null` by AS. It supplies an explicit nil for channel positions in the View-compiled movie. The [movie harness](../../tests/flashvm/result-channels_pair.as) publishes the exact compiled signature under `channel-results.dis`.
2. [The AS callback](../../tests/flashvm/result-channels_compose.as) sets the original box field to its value plus five. It returns the box, an array with two appearances of one integer queue plus nil, and a separate string queue. The C callback uses [`styxrpc_graph_diff`](../../reference/graph_diff.md) and the positional ref index, then writes current-group keys into the result child.
3. The provider commits the callback result and updates together. Every live channel leaf receives the new module token. The client changes the returned alias and observes that original box. Receiving 71 through an integer alias and `"label"` from the other route proves that the two keys have separate queues. A later write through one alias is read through the other.
4. The next attempt deliberately produces an invalid result after staging a ref update. AS refuses a descriptor with nine slots. The C device stages one key under incompatible element types. The caller sees a remote error and its original box stays at 41. A third call returns only nil channel leaves and applies its update to 46 without acquiring a channel lease.

## Use both directions after the callback

The [buffered initializer](../../tests/flashvm/result-channels_buffered.as) retains its owner endpoint and returns it from the string function. It successfully queues `"hello"`, then observes false when attempting a second send into the same one-slot outgoing FIFO. The Limbo client reads that seed and sends `"from-dis"` in the other direction. Between frames the movie takes it from the incoming FIFO and sends `"echo:from-dis"`.

The [rendezvous initializer](../../tests/flashvm/result-channels_meeting.as) creates zero slots. A Limbo `alt` has a send on this result and an idle branch on a separate local channel. Only the result branch completes. A second `alt` receives the sum 47. The owner keeps the computed value while its send returns false, and discards it after confirmation. The fixture uses nonempty text and nonzero integer values for its View predicates. In ordinary AS2, compare a polled result with `undefined` to distinguish idle from a received null, zero or empty string.

```limbo
# The idle local channel stays unready. The owner matches the remote send.
idle := chan of int;
alt {
sync <-= 42 => ;
unexpected := <-idle => raise "idle branch selected";
}
```

[The C device](channel_results_device.c) builds its module with the same signatures, owns the module registry, and drives the mailbox between calls to [`styxrpc_server_step_for`](../../reference/server_step_for.md). Each function fid owns the publication and has independent retirement. The guest releases its channel aliases. The final `retired` probe confirms both result resources are inactive. The device destroys the server first, drops the two owner references and destroys the module and carrier.

[Mailbox construction](../../reference/channel_mailbox_result_create.md) · [Typed module output](../../reference/channel_module_init.md) · [AS endpoints and composite results](https://github.com/bhgv/flashVM/blob/master/flashVM/doc/STYX_RPC_RESULT_CHANNELS.md)
