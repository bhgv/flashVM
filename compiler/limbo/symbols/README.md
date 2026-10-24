# Limbo symbol source maps

Version **0.1.3**. [Русский](README.ru.md).

Independent C99/CocoR reader of the **source-map prefix of limbo .sbl 2.1**. This is executable debugger infrastructure, not a complete Limbo compiler, Dis embedding or debugger UI. Types, functions, locals and globals after the PC prefix are deliberately opaque and not semantically validated. Older .sbl versions are unsupported in this first component.

## Build and check

Supply an external host generator. Both bhgv/coco-r-C and the C-output frontend of [bhgv/coco-r-limbo](https://github.com/bhgv/coco-r-limbo) are supported and tested:

```sh
cmake -S compiler/limbo/symbols -B build/sblmap \
  -DCOCOR_ROOT=/host/coco-r-limbo \
  -DCOCOR_EXECUTABLE=/host/coco-r-limbo/cocor
cmake --build build/sblmap
ctest --test-dir build/sblmap --output-on-failure
build/sblmap/sblmap program.sbl
```

The [.atg](sblmap.atg) is authoritative for LF/CRLF framing and counted module/file/PC structure. Its C99 actions call the [checked decoder](src/map.c). Generated files remain in the build directory; the external scanner/runtime is not vendored. Namespacing keeps generated globals separate from other frontend libraries. Native Limbo generator execution and a Limbo implementation of this decoder are not accepted by these tests.

## Prefix and output semantics

Read header, module spelling, file count/names, PC count and one source span/group per logical Dis PC. Source spelling is [file:][line.]position,[file:][line.]position followed by a space and the statement/block identity. Omitted file and line inherit state; the stop position updates state for the next start. File IDs are zero-based; source line/column zero is preserved as authored unknown data. The reader does not invent editor-coordinate conversion or a nearest-line breakpoint rule.

A complete physical terminator is required per line. Prefix text fields are limited to 1023 bytes; table limits are 4096 files and 262144 PCs, input 4 MiB, nonnegative numbers at most 2147483647. NUL is rejected throughout input. File spellings retain spaces/tabs/opaque bytes and are never opened as OS paths. No Unicode normalization or encoding validation is implied. Remaining tail lines are framed but their type/function/global syntax is not checked. prefix_bytes ends immediately after the PC table (or its zero count), before that opaque tail.

CLI output is tabular module/files/pc/prefix metadata, file rows and expanded entry rows. File values may contain tabs; consumers must retain the remainder after the first two separators. Exit 0 means a parsed map, 1 parse/output failure, 2 CLI/open/allocation setup error. A valid prefix is not proof that a complete .sbl or matching .dis is valid.

## C API and ownership

[Public C99 declarations](include/sblmap.h). The caller supplies accessible disjoint input, output and optional error storage. Parse calls share generated parser state and require external serialization; this is not a reentrant parser API. Successful maps contain independent copied strings/entries and no borrowed input. Read-only queries do not use parser globals; the owner must prevent concurrent destruction. No VM callbacks, Dis/FlashVM GC operations, sockets or filesystem reads occur in the library.

| API | Contract |
| --- | --- |
| [sblmap_parse](include/sblmap.h#L37) | Copy bounded input, parse into new ownership, publish *out only on SBL_OK. Caller retains any prior map; optional error buffer is cleared/written. SBL_INVALID, LIMIT, NOMEM or UNSUPPORTED preserve *out. |
| [sblmap_destroy](include/sblmap.h#L40) | Free a partial/complete owned map; null safe. All borrowed views expire. |
| [sblmap_module](include/sblmap.h#L42) | Borrow module spelling until destroy; null map returns null. |
| [sblmap_files](include/sblmap.h#L44) | Return file cardinality; null returns zero. |
| [sblmap_file](include/sblmap.h#L46) | Borrow one spelling until destroy; invalid index/map returns null. |
| [sblmap_count](include/sblmap.h#L48) | Return logical-PC cardinality; null returns zero. |
| [sblmap_prefix_bytes](include/sblmap.h#L50) | Return exact prefix extent; null returns zero. No claim about the tail. |
| [sblmap_entry](include/sblmap.h#L52) | Copy scalar source/group fields for one logical PC; invalid arguments or SBL_NOT_FOUND preserve caller output. |
| [sblmap_find](include/sblmap.h#L56) | Find the first exact start-file/start-line match at/after first_pc. NOT_FOUND preserves output; no fuzzy/spanning-line selection. |

SBL_OK is success; INVALID is malformed input/arguments, LIMIT an admission bound, NOMEM an allocation failure, UNSUPPORTED a non-2.1 header, NOT_FOUND an absent query result. Invalid argument diagnostics may be empty; status remains authoritative. Scanner fatal paths are converted to API failures. Exhaustive allocation-fault cleanup and LeakSanitizer are not accepted in this series.

## Evidence and remaining debugger work

Seventeen Python scenarios include 4096 independently compressed spans, all prefix truncations, state/file changes, CRLF, malformed numbers/punctuation, limits, NUL, opaque tail and file spellings. The C embedding fixture checks retained ownership across 64 failure/success resets, output preservation and exact-line lookup. [Optional upstream writer test](tests/upstream_writer.py) compiles the actual upstream sblsrcconv function body in a bounded host harness and checks 4096 emitted spans. It is not a complete upstream compiler/VM boot.

```sh
python3 compiler/limbo/symbols/tests/upstream_writer.py \
  --upstream /host/Inferno-OS-bhgv --tool /host/build/sblmap
```

[Upstream references](UPSTREAM.md) distinguish Dis debug services from wm/Tk presentation. [Compiler direction](COMPILER_DIRECTION.md) records C/Limbo/CocoR and later AS requirements. The next debugger layers still need full symbol/type/local decoding, module identity/reload handling, existing Dis control adaptation, channel transport, JIT mappings and actual target execution.
