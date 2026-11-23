# Connecting FlashVM and DisVM through libstyxrpc

[Русский](STYX_RPC.ru.md) · [Import an int function](STYX_RPC_IMPORT_INT.md) · [Export an int function](STYX_RPC_EXPORT_INT.md) · [Import a managed function](STYX_RPC_IMPORT_MANAGED.md) · [Export a managed function](STYX_RPC_EXPORT_MANAGED.md) · [Launch and file operations](STYX_RPC_SESSION.md) · [Ref arguments](STYX_RPC_REFS.md) · [Reference graphs](STYX_RPC_GRAPH.md)

FlashVM and DisVM communicate through libstyxrpc over Styx. Supply an authorized,
connected carrier to the Styx SDK. A module publishes its compiled interface as
`.interface` in the mounted VFS. Each call uses a fresh function fid and exchanges
UniBus S-records. Values cross the connection. Each VM owns its own objects,
garbage collector and execution threads.

The scalar adapter maps ActionScript numbers to exact signed
`fn(int): int` imports and exports. An imported name is an ordinary callable AS
global. DisVM uses a normal typed Limbo `load` to load FlashVM-backed exports.
The adapter is in `flashvm_styxrpc`, and the independent libstyxrpc SDK is
usable from any C program. External and DisVM-side scalar clients use the same
interface verifier, signature and schema checks, and record format. Extending the
DisVM-side implementation requires preserving that common contract.

## Build

First install the hosted libstyxrpc SDK and its dependencies. With an installed
SDK at `/path/to/prefix`, configure FlashVM from the workspace:

```sh
cmake -S flashVM -B build/flash-styx \
    -DFVM_ENABLE_STYXRPC=ON -DCMAKE_PREFIX_PATH=/path/to/prefix
cmake --build build/flash-styx --target flashvm_styxrpc
```

Link the `flashvm_styxrpc` target in the application and include
[flashvm/styxrpc.h](../include/flashvm/styxrpc.h). For automated checks keep
`FVM_BUILD_TESTS=ON`, build `flashvm_tests_styxrpc_host` and pass its path as
`STYXRPC_FLASHVM_TEST_HOST` in the [libstyxrpc acceptance build](https://github.com/bhgv/flashVM/blob/master/lib/styxrpc/examples/README.md#disvm-integration-tests).
That test starts both VMs and checks both call directions and error recovery,
with DisVM in interpreter and JIT mode.

Use a dedicated FlashVM owner context for synchronous RPC. The adapter covers scalar `int` and managed copied values, including
strings, arrays, tuples and nil. The external client uses the same signature
and shape verification as DisVM. Acceptance covers fragmented Styx array writes,
Unicode with embedded NUL, and rejection before writing on shape mismatch
or a missing reverse PID. The AS session object adds named
Dis launch with independent wait and cancellation tickets, and positional stat,
read and write of Styx files. The host selects the reverse PID. The launched
program can use its delegated `.var`, `.fd` and `.vfs`. Ordinary ref arguments copy back to their original Arrays. The graph profile adds nested
and returned refs, shared identities and sparse copy-back. ActionScript bindings for
hybrid tables and channels, direct declaration of reverse resources from AS, and
migration of existing in-process editor bridges remain open.
