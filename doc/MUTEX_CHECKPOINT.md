# Mutex and synchronization-debugger checkpoint — v0.5

This checkpoint implements opt-in `mutex.new/list`, shared native ownership,
read-only state/origin, acquire/release/touch, script worker IDs, and shared
mutex/channel capabilities nested inside transported object graphs. It also
adds coherent mutex wait-for snapshots, cycle detection, AVM1 function/offset
and supplied source maps, channel wait inspection, queue freeze/resume and
non-destructive packet inspection. Numeric AVM1 table member access makes
`mutex.list()[0]` usable; indexed tables are not a complete standard Array class.

Contract, examples and boundaries: [English](MUTEX_CHANNEL_DEBUG.md) /
[Russian](MUTEX_CHANNEL_DEBUG.ru.md). API additions preserve value ABI 2.

## Executed validation

All builds below use strict C89/C++98, statically linked bundled libraries,
and Linux GCC/G++ 13.3. Tests/examples retain assertions in optimized builds.

| Profile | Passed tests/examples |
|---|---:|
| AGG, fonts and image codecs | 138 |
| Cairo, fonts and image codecs | 138 |
| AGG, image codecs, fonts disabled | 134 |
| Headless, fonts, image codecs disabled | 132 |
| Default AGG with ASan/UBSan, stated exclusions | 138 |

Five new tests and one compiled C89 example are included. In addition to the
full sanitizer run, the concurrent debugger test passed 25 repeated instrumented
runs. It covers primitive string lifetime, freeze behavior of native receives,
concurrent graph/capability delivery, and explicit resume after close.
Mutex stress protects 8000 increments across four OS workers. The deadlock test
creates a real two-worker mutex cycle and verifies both waiter and owner source
locations, stop, join and teardown cleanup. ActionScript tests execute AVM1 bytes,
not just the native methods. The `.as` example is illustrative source only.

Reproduce the default instrumented build using
`python3 scripts/check.py --sanitize --limited-asan-stack`.
The explicit final option is required in this restricted environment because
libasan cannot discover/unpoison unwound exception stack frames through `/proc`.
Stack ASan is disabled **only** in `src/capi/serialization_c.cpp`; heap/global
ASan and UBSan remain enabled there. `tests/container_exceptions.cpp` uses UBSan
only. LeakSanitizer is disabled. Full stack-ASan coverage and leak checking have
not been certified. The independent exception probe and rationale remain in
`doc/validation/file-sanitizer-exception-probe.cpp` and the previous checkpoint.
The final core/platform objects and every test/example were rebuilt; unchanged
vendor sanitizer objects from the full run were retained for the final relink.

Evidence is under `doc/validation/mutex-*`: final CTest logs, build definitions,
ASan/UBSan result JSON/logs, repeated concurrency result and source hashes.
`python3 scripts/check_vendor_sources.py` verified all 2094 vendored files.
Git whitespace checks passed. Packaging verifies ZIP CRC, extracted clean
checkout, complete Git integrity, matching HEAD, and private stash preservation.

## Not certified or not implemented

Remote Internet-address mutex origins/proxies, distributed ownership and
cross-process/file restoration of native capabilities are not implemented.
File transport rejects those resources without writing a record. Channel-only,
mixed channel/mutex, or join deadlocks are not proved; channel waits are exposed
for inspection, and the exact cycle detector covers mutex ownership waits.
Packet freeze stops dequeue, not all worker execution or live native resources.
Interactive debugger UI, automatic source reconstruction and a complete call
stack remain pending. Source maps must be provided by embedding tooling.

Capability wrapper decorations remain local. Native queue capability cycles
are rejected to avoid reference leaks; ordinary serialized graph cycles remain
supported. Mutex acquisition is nonrecursive and has no fairness guarantee.
`mutex.list` has interpreter scope, not lexical-scope analysis.

This version is not a declaration of complete SWF <=8/standard-object support,
JIT completion, or tested native Windows/macOS/Android binaries. Platform registry
adapters are supplied in `ports/`; only Linux was executed. The authoritative
internal Git history is included in `FlashVM-with-history-v0.5.zip`. The mirror
was not updated: no mirror-update command accompanied this request.
