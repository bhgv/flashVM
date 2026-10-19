# SWO1 and SWA1 formats

All integer fields are unsigned little endian, without C structure padding.
SWO1 carries AVM1, not FIR or serialized memory addresses.

| Offset | SWO1 field |
| --- | --- |
| 0 | Four bytes `SWO1` |
| 4 | u32 version = 1 |
| 8 | u32 requirements: bit 0 FlashVM channels; other bits rejected |
| 12 | u32 module-name byte length, 1..255 |
| 16 | u32 AVM1 byte length, 1..2 MiB |
| 20 | u32 FNV-1a of name followed by AVM1 bytes |
| 24 | ASCII module name, without NUL, then AVM1 bytes |

Name syntax: `[A-Za-z_][A-Za-z_0-9.]*`. FNV-1a starts at 2166136261 and
updates `(hash XOR byte) * 16777619` modulo 2^32; it detects accidental corruption,
not authentication. The final instruction is ActionEnd. Record boundaries,
Push payloads and branch targets are validated; unknown opcodes fail. This is
the current FIR backend's instruction profile, not all of AVM1.

SWA1: four bytes `SWA1`, u32 version=1, u32 count (1..1024), then count records
`[u32 SWO1 size][SWO1 bytes]`. Trailing bytes fail. Module names identify v1
initializers; function-call relocations are not defined yet. SWF8 contains one
DoAction per object, then ShowFrame and End. Intra-object jumps need no changes.
