# Deep mount fixture: stop request and opened-fid retirement

Copyright (c) 2026 bhgv

[Русский](MOUNT-RETIREMENT.ru.md) · [Native libraries](../README.md) ·
[Union/module integration](../../runtime/VFS-TRAVERSAL.md)

## Problem and resulting behavior

The deep native mount fixture previously set its main-loop exit flag inside the
`stop` Twrite callback. After Rwrite, the mount client still had opened file and
control fids and attempted their normal Tclunk cleanup. The fixture could leave
its dispatch loop and close the connection before those messages completed.
Its relay still correctly rejected a truncated header; ignoring that error or
retrying a side effect would hide the ordering defect.

`stop` now records **a shutdown request**, not immediate teardown. The sole client
can read its opened file and receive Tclunk replies. Original libstyx retires all
remaining fids on disconnect and invokes `freeclient`; only that callback selects
main-loop exit. Main subsequently calls styxend outside the callback. This is a
single-client test fixture, not a new production-service shutdown policy.

Only the fixture and its regression driver change. DisVM execution, VFS selection,
C library APIs, canonical Inferno snapshots and 9P messages remain unchanged.
There is no VM pause, guest cooperation, enlarged timeout or relaxed frame check.

## Ownership and sequence

1. The client walks sixteen directory components, then the remaining directory
   and file. Intermediate unopened fid cleanup stays separate from opened fids.
2. It opens the file readonly and ctl write-only. The fixture must eventually
   report exactly two opened-handle closes and one final client retirement.
3. One Twrite with `stop` at offset zero acknowledges exactly four bytes.
   The callback stores no input and does not invalidate either opened object.
4. A subsequent real Tread must still return `depth` from the same file fid.
   This makes premature teardown deterministic instead of depending on scheduling.
5. In `clunk` mode both opened fids receive explicit Tclunk acknowledgements.
   In `disconnect` mode the file is clunked and connection EOF retires ctl.
6. The original freeclient ordering deletes fids before calling the fixture's
   callback. That callback only records retirement and requests loop exit; it
   never calls styxend reentrantly.

The driver splits request prefixes deliberately, validates response sizes/types/
tags/counts, and bounds startup/socket/child waits. It never treats transport EOF
as a successful Tclunk. Process cleanup kills and reaps an unfinished fixture on
failure. Neither driver mode retries a rejected operation.

## Sources

- [Deep C server and callbacks](../tests/mount/deep.c): `writefile` sets
  stop_requested; `freeclient` selects stopping after fid retirement.
- [Independent 9P regression driver](../tests/mount/retirement.py): exact replies,
  retained read, explicit clunk versus disconnect and final lifetime counters.
- [Existing native mount client](../tests/mount/client.c) and
  [strict fault relay](../tests/mount/wire.py): unchanged behavior and checks.
- [CTest registration](CMakeLists.txt): two new bounded retirement cases.

All implementation/test-server changes are C; the independent packet driver is
Python, as permitted for examples and tests.

## Reproduce

From the repository root, using the current pinned native sources explicitly:

```sh
cmake -S disVM/native -B build/mount-retirement \
  -DINFERNO_SOURCE="$PWD/disVM/native/src" \
  -DINFERNO_HOST_SUPPORT="$PWD/tools/limbo-bootstrap" \
  -DINFERNO_VFS_SUPPORT="$PWD"
cmake --build build/mount-retirement --target \
  vfs-mount-deep-server vfs-mount-test server-loopback-test \
  vfs-file-wire-test vfs-peer-wire-test vfs-mount-control vfs-event-resource-server -j4
ctest --test-dir build/mount-retirement \
  -R '^(vfs-mount-|vfs-mux-provider-)' -j4 --output-on-failure
ctest --test-dir build/mount-retirement \
  -R '^(vfs-mount-retirement-|vfs-mount-deep$|vfs-mux-provider-deep$)' \
  -j4 --repeat until-fail:30 --output-on-failure
```

The first selection contains 36 mount/mux/control cases, not the entire SDK.
The repetition runs each of four selected cases 30 times, stopping at the first
failure; it is not retry-until-pass. Explicitly building all listed targets avoids
mistaking missing fixture binaries in an old build directory for protocol failures.

For instrumentation, use another build directory with
`-DCMAKE_C_FLAGS='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'`
and `-DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'`, then run CTest with
`ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1`. Leak scanning remains
unavailable in this sandbox; this is ASan/UBSan evidence, not LSan acceptance.

## Evidence and remaining limits

Linux amd64: the original fixture fails the post-stop read with Broken pipe,
connection reset or early EOF. The corrected fixture passes both direct modes,
36/36 mount/mux/control cases in both normal and ASan/UBSan builds, and
120 repeated concurrent executions.

The earlier intermittent deep relay error is consistent with this reproduced
shutdown defect. This does not prove it was the only possible source of every
historic transport failure. Grant checkpoint variants7/11 remain a separate
stability investigation; no full parallel SDK success is claimed here.
