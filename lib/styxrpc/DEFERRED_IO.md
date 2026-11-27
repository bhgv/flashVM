# Deferred file transfers on a C provider

[Русский](DEFERRED_IO.ru.md) · [SDK guide](README.md) · [C and Limbo walkthrough](examples/disvm/DEFERRED_IO.md)

A callback tree can serve a slow file while the rest of its Styx session stays
responsive. Register a [selector](reference/server_enable_deferred_io.md) on the
ordinary server before dispatch. For each open or created file, it chooses
which read and write callbacks run on native workers. The dispatcher can then
answer requests on independent fids while a selected callback waits.

The server copies a write request before starting its worker and supplies a
private read buffer. One worker owns the transfer on each fid. The original
9P dispatcher publishes the reply and handles `Tflush`, clunk and connection
loss. A callback uses [styxrpc_server_io_cancelled](reference/server_io_cancelled.md)
to observe that its exact request has retired and return promptly. A result
that arrives after retirement is discarded.

Open, close, metadata, enumeration, mutation and unselected transfer callbacks
run on the server owner. A selected worker can overlap them on other resources.
If cookies or owner callbacks share mutable state, synchronize that state in
the provider. The writer's buffer and the reader's output belong to one
callback and remain valid through its return. A clunk can acknowledge before a
worker exits. The adapter holds its cookie until the worker joins and invokes
close on the owner.

Both providers use ordinary Styx data replies and one tag-specific `Tflush`.
DisVM's reverse provider takes cancellation through its native wait scope.
A mounted Limbo caller drives the new standalone C provider with
[Processctl.cancelwait](https://github.com/bhgv/flashVM/blob/master/disVM/module/processctl.m) on a direct child.
Already acknowledged bytes remain in effect. A write without a reply may
have taken effect, so the caller should check current state before issuing
another mutation.

The owner admits up to 16 active or retiring workers for one server. Each
worker slot stays occupied until join. Selected operations of zero length
complete immediately. Teardown sets the stop flags and joins outstanding
workers. It waits for provider callbacks to return, so opt in only file
operations whose waits can run cooperatively.

Run the [independent and paired examples](examples/disvm/DEFERRED_IO.md) to
verify short writes, individual cancellation, continued progress, fresh opens
and release of worker slots.
