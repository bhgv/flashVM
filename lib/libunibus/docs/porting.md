# Porting and embedding

Copyright (c) 2026 bhgv

[Guide](../README.md) · [API](index.md)

| Layer | Requirements | Boundary |
| --- | --- | --- |
| Styx::codec | C99, exact uint32/uint64,8-bit bytes, byte/string primitives | Original 9P codec; no allocator/OS/threads |
| Unibus::wire | C89,8-bit bytes, caller buffers, portable32-bit limbs | Existing framing/state machines; no allocator/OS/threads |
| Hosted Styx | Linux64 libc/libm/pthread | amd64 acceptance; arm64/riscv64 source selection needs target testing |
| Unibus::queues | C99, malloc/free, pthread | Native waiting/reordering, not an interpreter scheduler |

For an MCU, first build the minimal codec target. The application provides physical read/write/wait/poll, memory, session/fid ownership and callback policy. Never change wire endian/field widths to match native types. DMA/ISR callbacks must not call blocking hosted functions; transfer ownership to a normal service worker. SPI/I2C/CAN/radio can need a packet/reliability adapter before an ordered-stream protocol.

Complete hosted lib9 is not claimed as an already tested freestanding/32-bit port. The copied library requires no VM, GUI or sibling repository. Viewer/reference tools are optional for linking. The installed CMake SDK owns namespaced targets and include paths.

## Source and licenses

SOURCES.jsonl records the source path and prepared-algorithm hash before layout. Export checking compares executable tokens after readable_parts expansion; comments/layout and inserted braces are not algorithm edits. This does not replace behavioral tests. Original Inferno licenses remain separate from bhgv MIT additions.
