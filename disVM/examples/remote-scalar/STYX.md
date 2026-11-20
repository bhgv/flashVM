# Load a published scalar Dis library over Styx

[Русский](STYX.ru.md) · [Client source](styx-client.b) · [Styx route](../../native/terminal/SCALAR-ROUTE.md) · [Transport API](../../module/STYXSCALAR-CALL.md)

The caller imports the original `compute.m`, loads `Compute` from its usual
VFS path and calls `compute`. A publisher can place a generated Dis adapter
at that path. The adapter performs its request and reply through ordinary
open, write and read operations. Each direction uses its own granted
projection of a `.unibus` file.

## Prepare and exercise the publication

Build the [Coco/R generator](../../../compiler/limbo/frontend/README.md),
the [native Styx SDK](../../native/README.md) with VFS support, and the
[source-matched runtime and Limbo bootstrap](../../runtime/README.md) with
CMake's Unix Makefiles generator. Build `terminal-scalar-route` in the SDK.
Run the driver from the repository root, substituting your four build paths.

```sh
python3 disVM/native/terminal/tests/scalar/guest.py \
  build/scalar-frontend/limbo_remote_owner build/scalar-runtime/disvm \
  build/scalar-runtime/bootstrap/limbo-bootstrap build/scalar-sdk
```

The driver generates the adapter from `compute.m`, compiles it to
`implementation.dis` and compiles the local `styxscalar.dis` helper and
caller. It mounts the restricted request and reply files before starting the
caller. The two original Styx sessions share one bidirectional carrier. The
owner fixture checks the request and sends a reply before acknowledging the
write. The driver starts a fresh route pair for each case, checks exactly one
owner invocation, and joins both sessions after DisVM shutdown. Expect
`PASS Styx Compute value`, `error` and `missing` in interpreter and JIT modes.

The [generator command](../../../compiler/limbo/frontend/REMOTE-OWNER.md)
takes the route paths and tags at publication. Each deployment must supply a
fresh reply inbox and enforce one outstanding call for each route pair.
The owner in this example is a native test method. Automatic owner discovery
from an arbitrary mounted `.dis` and invoking owner Limbo over this route
still require runtime integration.
