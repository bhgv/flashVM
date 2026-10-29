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

## pilot.c: genuine module/type/code payloads

[pilot.c](pilot.c) is the second, fully line-commented C99 tutorial. It constructs
the exact shared 180-byte Pilot golden with two descriptors: module data4/align4
and frame64/align8, both full scalar map00. The metadata conditionally includes
data type0 and executable entry0/frame type1. The pool holds the five Rune name
Pilot at ID0. Code count2 contains checked signed30 MOVW immediate42 to MP[0],
then operand-free EXIT. Initializer count0 means runtime default data, not an
absent data object. All input/output storage is disjoint and owner-local.

Build target `dis64_pilot_example`; run it with one explicit output filename.
Use `dis64inspect file --pilot` or installed `run-dis64-pilot.sh` to validate
all supported semantic links. No input file is executed. Validation precedes
opening output; late fwrite/fclose errors report failure without rollback.
The SDK includes the independent pilot42.hex and every required shared header.
Detailed commands and current execution boundary: [tool guide](../README.md#real-payloads-explicit-scalar-pilot).
