# C99 Dis64 envelope example

[Русский](README.ru.md) · [Every-line commented source](main.c) ·
[Build/API contract](../README.md)

The program writes an explicitly requested file using the shared selected
Dis64 envelope and string-pool headers. It creates a lossless Rune record,
keeps duplicate content under two pool IDs, checks readback/comparison, stages
four sections, validates their directory and only then opens the destination.
Every code line explains its role or ownership.

The M/C/T payload markers intentionally provide no executable module semantics.
Inspecting the result reports `execution_supported:false`. The useful outcome
is a reproducible byte container and a working public C99 codec path.

The complete guide gives both source-build and relocated SDK commands. All
buffers are stack-owned, borrowed views expire before return, and the output
stream is closed after success or failure. Codec rejection happens before file
opening; final write/close errors return failure without filesystem rollback.
