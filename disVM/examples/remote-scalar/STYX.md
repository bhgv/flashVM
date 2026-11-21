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

## Copy-back from an original Dis export

The same `owner_guest.py` command compiles [the shared interface](ref.m),
[the caller](styx-ref-owner.b) and [the provider implementation](ref-owner.b).
The caller uses ordinary `load Refexample "/peer/mounted/ref.dis"` and invokes
`revise`. The return tuple carries the owner's call counter and the original
`big` field. The provider changes both fields and the caller checks them
through a second reference to its object. It also checks that an exception
after the provider's writes leaves the caller's fields intact, that a nil
reference is rejected locally, and that the next call observes the counter
in the same loaded module. Expect `PASS mounted original ref Dis` in both
execution modes. The driver closes both VM processes and their temporary roots.

## Call with three arguments

The `owner_guest.py` command also compiles [blend.m](blend.m) with
[the client](styx-list-owner.b) and [the original provider](blend-owner.b).
The client loads `/peer/mounted/blend.dis` and calls
`blend(40, (-2, min), max)`. The provider receives three arguments and
adds the two narrow fields and the two wide fields. The client assigns
the nested return to `(count, (narrow, wide))` and checks `(1, (38, -1))`.
The hosted compiler embeds
[the resolved types](../../../tools/limbo-bootstrap/STYX-TYPE-TABLE.md)
in the owner image. The driver reads the lists from `.interface`. It checks
that the same loader rejects a copy whose embedded signature was altered.
A zero seed raises an error. The next successful call verifies the
original instance kept its call count. Expect
`PASS mounted original argument list Dis` with and without JIT.

## Pass one object in two ref arguments

The same `owner_guest.py` command compiles [ref.m](ref.m) with the
[caller](styx-refpair-owner.b) and [original owner](refpair-owner.b).
The caller loads `/peer/mounted/refpair.dis` and calls `exchange` with
the same object in both positions. The owner increments the first
argument's `narrow` by two, reads through the second, then increments
the second by three. Starting at 7, the two increments leave one
object at 12. A second call passes two objects with identical field
values and verifies independent updates. The following calls verify
exception isolation, local nil rejection and the owner's persistent
call counter. A final call uses two test marker files to make a helper
in the caller change the second object after the owner received both
snapshots. It verifies that the conflict preserves the local edit,
the first object and the previous return values. The test driver relays
the markers and removes its temporary roots. Expect
`PASS mounted original ref pair Dis` in both modes.

## Use the owner's actual compiled profile

The `owner_guest.py` command also pairs
[styx-loaded-profile.b](styx-loaded-profile.b) with an
[alternative original implementation](blend-scalar-owner.b). Both
the ordinary and alternative implementations export `blend`, but
their compiled signatures differ. The owner links the alternative
through the [shared service](../../runtime/SCALAR-OWNER-SERVICE.md#profile-from-the-original-load).
The client tries the three-argument import first and gets nil. At the
same VFS path it then loads the actual `fn(int): int` interface and
gets 42 from `blend(40)`. Both JIT and interpreter mode should print
`PASS Styx profile from loaded export`.

## Load six exports that share state

Run the same `owner_guest.py` command. It builds
[the interface](loaded-set.m), [the owner](loaded-set-owner.b) and
[the client](styx-loaded-set.b). The owner publishes the selected names
from one original image. The client uses ordinary `load` of
`/peer/mounted/set.dis` with both the full interface and a subset.
Calls alternate between `int`, `big`, a tuple, one `ref`, a nested
argument list and two aliases of the same object. They observe the same
counter through both imports, including after an owner exception. A
second run reserves a missing export and checks that even the valid
subset fails to load. The driver closes the carriers and removes the
temporary roots. Expect `PASS mounted loaded export set shares state`
and `PASS loaded export set refused as a whole` in both execution modes.
