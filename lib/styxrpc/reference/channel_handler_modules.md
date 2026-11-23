# Host functions with channel arguments

[Русский](channel_handler_modules.ru.md) · [SDK](../README.md) · [Lend local argument queues](channel_argument_scopes.md) · [Publish result channels](channel_leases.md)

A hosted C application can receive channel arguments using the native scoped Dis profile. Dis retains each channel and its storage. The passed key and caller PID give the handler a route to the live queue or rendezvous for this invocation. The dispatcher serves the function files. The application owner can safely use the reverse connection during a callback because the dispatcher acknowledges the write and handles polling independently.

1. Define the interface in Limbo, for example `relay: fn(port: chan of int): int;`. Obtain the exact compiled signature with Limbo's `-S` output. Create a `StyxRpcChannelFunction` with that name and signature, the schema `{"(c[i])", "(i)"}`, and a C callback. For a successful copied int result write one child using [the integer encoder](../include/styxrpc/managed.h) with `STYXRPC_VALUE_INT`. For void set `*used = 0`.
2. Use [create](channel_handlers_create.md) to construct the module. Register `styxrpc_channel_handlers_tree` with [the Styx server](server_create.md) on the forward side of an authorized bidirectional connection. Mount the reverse client on the other side of that same session.
3. Run [step](channel_handlers_step.md) on the application owner. The SDK checks the frame, PID and types, coalesces aliases, and invokes the callback once per completed FID. In the callback, select a key from `call->grants` and use [channel exchange](channel_exchange.md) on the same reverse client to send, receive, or use a guarded selection. The SDK validates the copied result and publishes it on the function FID. The caller polls the FID while waiting.
4. When the forward server ends, destroy it and [stop](channel_handlers_stop.md) the module to wake the owner. Join both sides before you [destroy](channel_handlers_destroy.md) the module. Release the reverse client after the last running callback returns.

An incomplete call never runs. Closing a queued function FID removes its job. Closing an active FID detaches the reply and lets the callback finish. A completed reverse operation may already have changed the caller's queue. Choose a finite reverse wait budget. An application callback may be able to do other work after its caller has disconnected.

The module accepts up to eight distinct channel keys per call, including aliases and nil. Each element and any copied result uses only value types. The module exposes `.interface`, `.ref-fields`, and `.unibus-version`. For the module that issues leased result channels, use [the result channel module](channel_module_init.md).

The [standalone C check](../tests/channel_handlers.c) constructs a fragmented request and verifies concurrent dispatcher/owner lifetimes, cancellation before execution, and a valid copied reply. After [building the hosted SDK](../README.md), run:

```sh
ctest --test-dir build -R '^styxrpc-channel-handlers$' --output-on-failure
```
