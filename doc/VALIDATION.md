# SWF 8 development checkpoint validation

2026-10-11: 94 tests/examples passed in strict Linux C89/C++98; 93 under
ASan+UBSan and the container exception test under UBSan only. LeakSanitizer
remains disabled by the managed runtime constraint documented below.
CMake 3.31.10 Release/CTest passed all 94 tests/examples; the configuration
without AGG/OS input-network/render jobs passed 90. Library-only minimal
configuration built successfully. Enabling unavailable JIT was correctly
rejected. Logs/manifests: `doc/validation/phase2-*`.

Windows/macOS/Android and the six JIT targets were not validated. Cairo is
not integrated. Full SWF 8 compatibility is not claimed; see SWF8_ROADMAP.
Earlier evidence below is historical and its CMake-unavailable statement no
longer describes this checkpoint.

# Validation and remaining work

## Reproduce

On Linux with Python 3, GCC/G++, ar and timeout installed:

```
python3 scripts/check.py
python3 scripts/check.py --sanitize
```

The runner compiles all dependency-free runtime sources and Linux adapters into
a static library, then builds/runs every native regression test and embedding
example (except optional system SQLite). C uses `-std=c89`, C++ uses
`-std=c++98`, both use `-Wall -Wextra -pedantic-errors`. Tests have a 30-second
process timeout. Assertions are enabled. CMake configurations also explicitly
keep assert-based tests enabled in Release; the minimum CMake version is now 3.1,
matching the already-used `target_sources` command.

Normal CMake workflow on a machine with CMake:

```
cmake -S . -B build -DFVM_ENABLE_SQLITE=OFF
cmake --build build
ctest --test-dir build --output-on-failure
```

CMake was unavailable in the execution environment, so the release was verified
with the standalone compiler runner, not with CMake/CTest. Default SQLite is OFF;
the optional system-SQLite build is not part of this verification and its sources
are not bundled. No external source download or new dependency was required.

## Results

89 tests/examples passed in the strict Linux build. The sanitizer run verifies
88 with AddressSanitizer + UndefinedBehaviorSanitizer and the exception-injection
container test with UBSan alone. Machine-readable manifests in `validation/`
record test filenames, compiler version, flags and explicit exclusions. The
compiler is GCC 13.3.0 on Linux. Android/macOS thread adapter sources also compile
with strict C89 on the Linux host; that is a syntax check, not platform execution.

LeakSanitizer cannot enumerate tasks in this managed runtime (`/proc` access is
unavailable). It is disabled explicitly. ASan also reports an unavailable stack
top while handling nested C++ rethrows; an independent exception-only probe
reproduces that runtime warning. The vector-constructor exception-injection case
then triggers a report within ASan's stack-unpoisoning path. This test is therefore
built/run with UBSan only, retaining its live-instance and state assertions. Do
not interpret this run as proof of leak freedom or full ASan exception coverage.

## Regression evidence

| Area | Evidence |
| --- | --- |
| Cyclic/deep graphs | Existing gc_cycles; gc_deep_graph: pin/trace/sweep 12,000 cyclic objects |
| Bounded finalization | gc_finalizer_snapshot: newborn graphs, errors, one-shot callbacks, teardown reentry |
| Script finalizers | gc_script_finalizer: named AVM1 callback, resurrection, later reclamation |
| Native roots | gc_api_stack_capture and gc_callback_captures: value/stack/callable/metamethod captures, replacement |
| Userdata | gc_userdata: resurrection delays release, one-shot cleanup, coherent diagnostics and teardown |
| AVM1 live locals | actionscript_extensions: object-valued DefineLocal remains rooted across explicit GC |
| Native collector guard | Existing gc_native_reentry, gc_native_roots and nested-scope tests |
| Join/lifecycle | worker_join_concurrent; worker_context: concurrent/self-join and published current-worker handle |
| Close/backpressure | channel_owned and channel_close_races: full-queue trySend, owned string lifetime, close races |
| Snapshot transport | channel_graph_transfer: cycles, aliasing, prototypes/indexes, source destruction, native-resource rejection |
| ActionScript bridge | actionscript_extensions: actual AVM1, shared channel, input snapshots, graph/string results, cached roots, multi-join |
| Cancellation | actionscript_extensions: loop, receive and nested join cleanup via GC |
| Container failures | container_exceptions: constructor/reserve/push and map-copy unwinding with live-instance counts |

## Remaining work / scope limits

- Windows, native macOS and Android runtime/build CI are not verified here.
- Interned/copied VM strings remain valid for the context lifetime and are freed
  at teardown; this release implements tracing **object** GC, not string-arena GC.
- Fault injection covers internal container assignments, not every VM allocation.
- Native stack locals and arbitrary userdata memory are not scanned. Use root
  scopes, pins, registered captures or traced object properties.
- Native callbacks cannot be forcibly cancelled; destroy must not race handle
  users. Cross-context object pointers remain forbidden.
- Workers transfer named function definitions, not arbitrary closures or native
  bindings. The interpreter remains an AVM1 subset.
- Channel snapshots are for one process; TCP/UDP/TLS, custom channel operators,
  socket serialization and a complete ActionScript compiler remain future work.
- Graph packets reject native resources. Native channel sharing uses explicit
  retained wrappers/worker arguments to avoid reference cycles of queued handles.
- The optional SQLite adapter, JIT, rendering/media backends and other unrelated
  baseline placeholders were not completed by this update.

Subsystem claims are limited to these tested contracts. Full FlashVM/Flash Player
compatibility and cross-platform production readiness are not claimed.
