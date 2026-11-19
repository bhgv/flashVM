# libstyx: what and why

Copyright (c) 2026 bhgv

[README](../README.md) · [API](index.md) · [Tutorial](tutorial.md) · [Examples](../examples/README.md)

## Resources as names

A file is a named resource, not necessarily disk storage. A sensor, database row, command endpoint, window or data stream can have a pathname. Clients walk names and open resources; the server determines what each name means. It need not pre-enumerate an entire tree. Access checks and each client's exported projection remain server responsibilities.

## Library layers

`Styx::codec` converts standard 9P2000 messages in caller-provided buffers without sockets, threads, allocation or DisVM. `Styx::server` adds the original hosted libstyx callback interface. `Styx::client` provides the existing native client. `Styx::carrier` separates physical I/O. The complete hosted distribution currently targets Linux64; portable codec support does not imply a complete MCU server port.

## Request lifecycle

Tversion negotiates dialect and msize; Tattach creates a root fid. Twalk resolves pathname components relative to a fid, and Topen opens the result. Tread/Twrite operate on the held fid and offset; another walk is not required before every operation. A directory Tread returns encoded Dir records for immediate children only. Tclunk releases a fid; disconnect must clean up all session-held objects. A name may disappear after opening: design held-object lifetime separately.

## Choosing this interface

Use 9P to publish a namespace through familiar file operations. On a small device, start with the codec and supply session/fid/transport adapters. On Linux, start with the callback server. The teaching servers need an application access policy before exposure to untrusted peers. A synchronous sample backend is not a universal asynchronous service.

## Reading order

Start with the [tutorial](tutorial.md), choose an [example](../examples/README.md), then consult the [function dictionary](index.md). See [protocol/lifetime](protocol.md) and [porting](porting.md) for integration boundaries.
