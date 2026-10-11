# Dependency status update

Bundled zlib and AGG sources now live in `external/`, as requested in the SWF 8
phase. See `external/README.md`, dependency-specific provenance/license/patch
notes and `doc/BUILD_PLATFORMS.md`. Both are compiled offline and tested on
Linux in this phase. Cairo is not integrated; its C11 requirement and pixman
transitive dependency still need resolution for strict C89/C++98.
SQLite remains optional system linkage. Existing mini_regex stays in `extern/`.
The earlier "no network access" statement below is historical, not current.

# External dependencies and offline builds

FlashVM aims to vendor verified upstream sources under `extern/`.
A dependency is **not bundled** unless its real source and licensing material
are checked in and an offline build has been tested.

| Dependency | Status | Requirements |
| --- | --- | --- |
| mini_regex | Bundled and compiled | Limited byte-oriented regular expressions |
| SQLite | Not bundled; optional system linkage | Official `sqlite3.c` and `sqlite3.h` amalgamation |
| zlib | Not bundled | Upstream source archive and license notice |
| AGG | Not bundled | C++98-compatible release and license verification |
| Cairo | Not bundled | Cairo source and transitive dependencies, including pixman |

The current build environment has no network access. Installed headers and
binary libraries cannot substitute for source releases. No placeholder files
are represented as vendored third-party code.

Integration steps: obtain pinned upstream source archives; verify checksums;
record license notices; add bundled/system/off CMake choices; test offline
builds; validate on Windows, Linux, macOS and Android.

## Hybrid table details

`src/details/hybrid_table.hpp` introduces a standalone C++98 container:
dense nonnegative integer keys in an array of presence-marked slots, sparse
integer and string keys in custom hash tables, and an independent FIFO queue.
`get` reports presence separately from the value. `size` counts keys, not
queued elements. Numeric indexing starts at zero. Sparse indices do not force
huge array allocations. Existing keys can be overwritten.

This is a **container building block**, not yet wired into AVM1 objects,
garbage collection, prototypes or metatable dispatch.
