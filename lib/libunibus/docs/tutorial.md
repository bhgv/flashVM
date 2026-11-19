# libunibus: from the first request to your service

Copyright (c) 2026 bhgv

[README](../README.md) · [API](index.md) · [Introduction](introduction.md) · [Examples](../examples/README.md)

1. Build and run `unibus-nested`. Read [nested.c](../examples/nested.c): the application owns all buffers, and this example creates no socket or thread.
2. Assign outer tag 7, schema example.sensor.v1 and version 1. Encode/apply control tag 0 before releasing client traffic. Check every codec result, capacity and consumed count.
3. Nest inner tag 1 inside outer tag 7. Feed the resulting bytes into sstream in fragments. MORE asks for continuation; it is neither a failure nor end-of-message. On a complete outer record, validate the binding and deliver its payload to that client.
4. Run [queues.c](../examples/queues.c). One consumer waits for its tag while another has a ready message. This checks independent waits. The application starts the service worker; each connection end owns its own RX loop.
5. Read the [gateway](../examples/bridge/README.md). main owns stdin RX, dispatch enqueues nested opaque 9P for a client, each worker owns its backend socket, and an output lock prevents interleaved reply bytes. Different clients may reuse inner tags and fids.
6. Negotiate ordering separately. The basic gateway has no sequence prefix. For the ordered profile use the existing prefix/packet/order API: uint64 sequence, 1–8 bytes, explicit length and CRC5, never a native struct. Do not silently change wire profiles.
7. Define shutdown and uncertain effects. Partial physical I/O requires continuation, not command replay. A lost reply does not prove a write never happened. Truncated S-record EOF is rejected. Simple bindings have limited reassignment behavior; ordered API barriers are a separate contract.

## Acceptance

```sh
ctest --test-dir build --output-on-failure  # Run the built examples and regression checks.
```

Check normal requests, short buffers, invalid names, fragmented input, disconnect and release of held objects. Test transport coverage does not replace testing your physical driver.
