# Development 0.3 checkpoint: file channels, graph codecs and value metadata

This checkpoint adds a concrete, tested subset of the requested serialization and metadata work. It does not complete the Flash object catalogue. Commit subject: `v0.3 Add persistent file channels and serializable value metatables`. Base internal commit: `8918bc1371cb2e51cf8d902a133abc80962b8b92`. ABI changes from 1 to 2; native clients must rebuild. The mirror was not updated in this step.

Implemented:

- Portable stdio adapter in `ports/file_port.c`, C89 file/raw-byte APIs, retained synchronized handles, `r`/`w`/default `rw`, UTF-8 Windows path branch, finite EOF handling and ActionScript file creation.
- Versioned endian-stable, CRC-checked graph snapshots shared by memory graph channels and files. Preserve cycles, shared references, prototypes, metatables, registered callbacks/accessors/methods and traced callback captures. Checked limits and rollback on malformed/unsupported state.
- Native codecs for Boolean, Math, TextFormat/class metadata and the implemented host TextField subset. TextField embeds its used font bytes, text, current/future formatting, geometry and flags; destination caches are rebuilt. Font-name conflicts reject restore.
- Metadata on all existing primitive value tags and object identities. GC marks those edges in VM roots, stacks, registers, locals and property graphs. C and ActionScript attachment/retrieval APIs; primitive property hooks and AVM1 arithmetic/comparison hooks. Invalid cross-context attachments/results are rejected.
- Executable C89 example, illustrative ActionScript example and RU/EN usage documentation. Separate writer/reader executables test persistence across process exit.

## Actual validation

Linux x86-64, GCC/G++ 13.3.0, CMake 3.31.10, strict C89/C++98 settings, static bundled dependencies. Checks were rerun after final code changes.

| Profile | Tests/examples | Result |
| --- | ---: | --- |
| AGG, fonts/images/SQLite enabled | 132 | Passed |
| Cairo, fonts/images/SQLite enabled | 132 | Passed |
| AGG without fonts | 128 | Passed |
| No graphics/fonts/image codecs | 126 | Passed |
| AGG sanitizer run with the explicit stack limitation below | 132 | Passed |

Logs, profile definitions and sanitizer metadata are in `doc/validation/file-*`. Source manifest: 2094 vendored files verified unchanged. `git diff --check` passed. Git integrity checked; existing unreachable blobs are informational and private stash history is preserved.

Regression tests exercise cyclic/aliased graph restoration after source VM destruction; cross-process Boolean/Math callbacks; primitive metadata snapshots; registered rooted `__index`; signed zero/NaN/infinity; invalid enum bytes; every truncation boundary of a graph packet; CRC/ref/prototype-cycle rejection; unknown callback rollback; TextField editing, measurements and contours after restored embedded fonts; font and allocation-counter rollback; file permissions, truncation/preservation, raw NUL bytes, EOF, close and legacy receive behavior. Existing GC/worker/channel/native prototype regressions also pass.

Full default ASan failed on C++ exception unwinding in this restricted environment. `file-sanitizer-exception-probe.cpp` independently reproduces libasan's stack-discovery warning and stack error without FlashVM; its log is retained. The same probe passes with GCC `--param=asan-stack=0`. Final command: `python3 scripts/check.py --sanitize --limited-asan-stack`. This explicit option disables **only stack ASan instrumentation in `src/capi/serialization_c.cpp`**; its heap/global ASan and UBSan stay enabled. Other VM/test sources keep their usual instrumentation. Historical `container_exceptions.cpp` remains UBSan-only, and LeakSanitizer is disabled because `/proc` is unavailable. Full stack-ASan coverage and LeakSanitizer are not claimed. No ASan/UBSan error diagnostic was ignored in the passing run.

Old profile build caches initially retained VM objects from ABI 1. All VM/client objects were rebuilt before the passing profile results. Use a clean rebuild when upgrading ABI. The existing GCC scoped-AVM1-root diagnostic can still occur; this is not a warning-free build claim.

Reproduction: configure a fresh build with `cmake -S . -B build-file-agg`, build, then `ctest --test-dir build-file-agg --output-on-failure`. For other profiles pass `-DFVM_DEFS_FILE=$PWD/doc/validation/file-DEFS-cairo.txt` (or the no-fonts/minimal definition file). Run the source manifest checker and the sanitizer command above separately. CTest fixtures order the process writer before its reader, including parallel runs.

## Remaining requested work

MovieClip and much of the SWF <=8 standard object catalogue are not implemented. All-object serialization therefore remains unfinished. Live worker/channel resources, extension namespaces, arbitrary userdata, stack callables and AVM1 function/closure environments lack portable codecs. Standalone TextFormat measurement still needs destination font registration. Destination TextField requires opting into its text bridge. ActionScript workers do not automatically install the text bridge or application callback registries. Generic hooks exist but are not integrated into every opcode/coercion; primitive copies have no individual `__gc` identity. Extended `<...>` literals and custom channel operators still need a compiler. Crash-atomic file transactions, inter-process writer locking and large-file adapters remain future work. Native Windows/macOS/Android execution, full Flash compatibility and requested multi-architecture JIT are not certified by these Linux tests.

The delivered filename is `FlashVM-with-history-v0.3.zip`. It contains tracked sources, static dependency sources, documentation/tests and the full internal `.git`, including existing private stash history. It contains no rebuilt binaries. Archive verification checks ZIP CRC, an extracted clean checkout, matching HEAD, full Git object integrity, stash presence and vendored source checks.
