# Standalone library acceptance

Copyright (c) 2026 bhgv

[English guide](README.md) · [Русская приёмка](VALIDATION.ru.md)

Acceptance environment: Linux amd64, GCC, CMake and Python3. The source-level
Linux arm64/riscv64 selections have not been executed on those physical targets.

| Check | Result |
| --- | --- |
| Styx hosted | 12/12: six original server regressions, portable codec, five catalog models |
| libunibus hosted | 11/11: nested records, independent native waits, four order regressions, queue initialization failures, gateway, adversarial two-client cases, gateway initialization failures and short/failed I/O |
| Cross-library wire integration | 5/5 catalog models through independently linked gateway |
| Isolated copies | Each package configured, built and tested outside its original source directory |
| Minimal profiles | Styx codec/wire without hosted adapters; libunibus wire without native queues |
| Relocated SDK | Installed prefixes moved; external C and C++98 consumers linked/ran codec, wire, native client and queues with CMAKE_INSTALL_INCLUDEDIR=api/include |
| Documented applications | Installed example sources built/ran as external applications in all four profiles; 16 downstream executables including C/C++98 probes |
| SDK discovery | Exact version accepted; incompatible major/minor and missing required components rejected; unavailable optional components accepted |
| Sanitizers | ASan/UBSan hosted tests plus five cross-library cases passed; LSan was disabled in this sandbox |
| Offline documentation | Both languages, local Markdown links, physical source anchors, live independent viewers and path confinement |
| Workspace documentation viewer | 8 existing regression tests passed with its declared Python dependencies |
| Canonical relationship | 224 Styx / 37 libunibus exported source files verified against prepared canonical algorithms |

Reproduction from the repository root:

```sh
python3 lib/export.py --check
python3 lib/tests/isolated.py /absolute/path/to/cmake
python3 lib/tests/isolated.py /absolute/path/to/cmake --sanitize
python3 lib/tests/documentation.py
python3 lib/tests/reference_scanner.py
python3 lib/tests/interoperability.py build/standalone-styx-final build/standalone-libunibus-final
```

The last command accepts two existing build directories. `isolated.py` copies the
packages under `build/`, performs their builds and records command output there.
It requires the matching `ctest` beside the supplied `cmake` executable.

## Limits of the evidence

Tests cover actual native libraries, an independent wire client, five server models
and their S-record exchange. Token comparison preserves algorithms through layout;
it does not prove semantic comment quality or absence of every defect. This is not
physical SPI/I2C/CAN/USB, every OS, a complete MCU server or all engine-broker features.
The exported libraries must not require VM sources/builds; main-project help links
are optional external references.

## Reviewed tutorial maintenance

The nested-record, native-queue and gateway C examples now carry authored operation-specific contracts. `python3 -m lib.tools.example_lines` checks their review markers and known stock prose; it does not manufacture comments or certify their semantics. Formatting preserves explicitly reviewed contracts. The remaining source inventory is metadata, not a statement that every inherited comment has received manual review.

C++98 acceptance reproduced the former native-client undefined reference before adding C linkage to the canonical client header and its checked export. Initialization failure injection checks both engine allocation failures, barrier initialization and thread creation; constructed resources must retire in all four cases.

## Gateway failure isolation

The gateway edge suite checks strict CLI/two real ports, partial startup, a genuinely
pending RPC while another client progresses, FIFO-local malformed-client errors,
normal 9P Rerror passthrough, sticky backend loss/truncation/invalid length, malformed
outer traffic and output-loss wakeup with stdin still open (also with another RPC
pending). Native wrappers verify staged worker cleanup, remaining-byte handling
across EINTR/short counts/EOF, backend timeout versus stdout backpressure and terminal
output after a partial failed record. The output-reuse test fails against the original
io.c for that specific defect and passes after latching the failed direction.

Documentation checks verify not just anchor range but function names and exact
call-site excerpts at the linked source line. Reference scanner fixtures exclude
comment/string decoys from required-header and example selection.
