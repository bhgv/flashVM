# libunibus: what and why

Copyright (c) 2026 bhgv

[README](../README.md) · [API](index.md) · [Tutorial](tutorial.md) · [Examples](../examples/README.md)

## Independent conversations over one connection

libunibus carries messages for different clients over a shared connection while preserving record boundaries and tag namespaces. It is a communication library, not a filesystem or interpreter. Payloads can carry 9P, application commands or another negotiated format. File publication needs a separate 9P server; libunibus does not depend on libstyx.

## Layers and ownership

`Unibus::wire` provides C89 codecs and incremental state machines over caller-owned buffers, without allocation, OS or threads. Optional `Unibus::queues` adds C99/pthread queues and ordering. The application owns the physical carrier, service thread and client lifetimes. Queue APIs do not themselves launch a universal network broker.

## Two levels of tags

Transport tag 0 belongs to connection service: assignments, schemas and control operations. Other outer tags route clients. A client payload may contain nested S-records: clients A and B may both use inner tag 1. Clients receive their own nested messages, not access to tag 0 or shared RX. A worker waits for its queue without stopping RX for other clients.

## When to use it

Choose wire alone for a small controller with its own event loop; select optional queues for native waiting consumers. A simple 9P connection need not add multiplexing. The gateway demonstrates two independent queues and backend connections, not all features of the DisVM/FlashVM broker. Extra physical lanes, load policy and authorization remain separate integration responsibilities.

## Reading order

Start with the [tutorial](tutorial.md), choose an [example](../examples/README.md), then consult the [function dictionary](index.md). See [protocol/lifetime](protocol.md) and [porting](porting.md) for integration boundaries.
