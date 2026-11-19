# Protocol and lifetime

Copyright (c) 2026 bhgv

[Guide](../README.md) · [API](index.md) · [Examples](../examples/README.md)

Unibus uses the existing canonical S-record profile. The outer service owner handles tag0; independent client envelopes contain their own nested records. The same inner IDs can be reused by different clients. The ordinal prefix is a CRC5-protected width byte plus1..8 unsigned bytes. A sequence counter belongs to a tag/direction and renews only through its acknowledged boundary, not a new generation field in every packet.

## Ownership

Codec storage belongs to the caller. A decoded view cannot outlive its buffer. Successful opens/fids and native contexts have explicit references; removing a name does not reopen the resource. Cleanup/free require borrowing calls to have retired. Native structs/pointers are never serialized directly.

## Errors and partial I/O

Distinguish malformed input, temporary unavailability, EOF, confirmed prefixes and uncertain effects. Never automatically replay an uncertain write. CRC is not authentication. Bounds are checked before delivery to the application; a codec grants no authority. Local buffer/allocator exhaustion is an explicit failure, not fabricated EOF.

## Threads and scope

A separate service owner receives records; a consumer queue is not the whole physical channel. The library does not pause execution for inspection. Each mutable caller-owned codec context needs its own serialization; independent contexts can execute concurrently. Native queues provide their mutex/condition. The Styx example backend is synchronous and consequently delays its example dispatcher.

## Main-project help

- [Original native libraries](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md).
- [Ordered Unibus transport](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md).
- [Inferno Styxservers reference](https://powerman.name/Inferno/man/2/styxservers.html).
