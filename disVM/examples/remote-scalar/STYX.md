# Load a published scalar Dis library over Styx

[Русский](STYX.ru.md) · [Client source](styx-client.b) · [Styx route](../../native/terminal/SCALAR-ROUTE.md) · [Transport API](../../module/STYXSCALAR-CALL.md)

The [mounted function example](../../native/terminal/SCALAR-FUNCTION.md) shows the file-per-export protocol with a native callback.

The caller imports the original `compute.m`, loads `Compute` from its usual
VFS path and calls `compute`. A publisher can place a generated Dis adapter
at that path. The adapter performs its request and reply through ordinary
open, write and read operations. Each direction uses its own granted
projection of a `.unibus` file.

## Prepare and exercise the publication

Build the [native Styx SDK](../../native/README.md) with VFS support, and the
[source-matched runtime and Limbo bootstrap](../../runtime/README.md) with
CMake's Unix Makefiles generator. Build `terminal-scalar-route` in the SDK.
Run the driver from the repository root, substituting your three build paths.

```sh
python3 disVM/native/terminal/tests/scalar/guest.py \
  build/scalar-runtime/disvm \
  build/scalar-runtime/bootstrap/limbo-bootstrap build/scalar-sdk
```

The driver compiles the caller, publishes the original and its profile
with `dvm_module_publish_scalar`, and binds the publication to
`implementation.dis`. It mounts the restricted request and reply files before
starting the caller. The loader provides a native entry without caller adapter
bytecode. The two original Styx sessions share one bidirectional carrier. The
owner fixture checks the request and sends a reply before acknowledging the
write. The driver starts a fresh route pair for each case, checks exactly one
owner invocation, and joins both sessions after DisVM shutdown. Expect
`PASS Styx Compute value`, `error` and `missing` in interpreter and JIT modes.

The [generator command](../../../compiler/limbo/frontend/REMOTE-OWNER.md)
takes the route paths and tags at publication. Each deployment must supply a
fresh reply inbox and enforce one outstanding call for each route pair.
The first driver uses a native test method. The separate DisVM example below
loads and calls the original owner implementation.

The paired test binds an empty directory over `/scalar` after the first load.
Both that instance and one loaded after the bind reach the same owner using
the exact files retained by the publication. The final call uses the second
instance and checks that the owner kept its state.

## Run the owner in a separate DisVM

Both sides use a compatible profile from the original `.m`. The owner
runs the shared [scalarowner.dis](../../runtime/SCALAR-OWNER-SERVICE.md).
The host uses `Launchcontext` to pass the implementation path, export name,
request and reply paths and the two route tags.

Start the owner with a readable request inbox and the caller's writable
reply file. The caller gets its own readable reply inbox and the owner's
writable request file. The grant paths may have the same names in the two
namespaces while referring to opposite directions. Both peers use the same
request and reply tags. The owner loads the implementation once, reads successive
requests and returns one value or error record for each.

```sh
python3 disVM/native/terminal/tests/scalar/owner_guest.py \
  build/scalar-runtime/disvm \
  build/scalar-runtime/bootstrap/limbo-bootstrap build/scalar-sdk
```

The test starts two separate DisVM processes with separate VFS roots and
Styx sessions over a loopback TCP connection. The owner loads `implementation.dis`, which loads
[privatevalue.dis](privatevalue.b) and reads the owner's `provider-secret`
through it. The caller has a file with the same name and a different value.
A successful call returns 142. A call with -1 returns an error record.
Removing the owner's dependency also returns an error, even with the dependency
present at the caller. All three cases run with and without JIT. The owner starts after the request path
is ready so the caller exercises a pending reply. Each case closes its two
sessions and reaps both processes.

In the [sequential session](../../native/terminal/SCALAR-SESSION.md), one
instance returns 142, then an error, then 144. The last value verifies that
the implementation retained state across the error. Mounting an arbitrary
remote `.dis` still needs an adapter publication or runtime owner discovery.

In the two-VM test, the owner publishes its actual `implementation.dis` over
[Styx](../../native/terminal/UNIBUS-SERVER-FILE.md). The caller mounts the
original and `.unibus` from the same session, pairs the original with its local
adapter, and reads the original bytes through the same bound path used by `load`.
