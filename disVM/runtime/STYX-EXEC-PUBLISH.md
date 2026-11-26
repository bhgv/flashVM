# Publish executable VFS nodes from Limbo

[Русский](STYX-EXEC-PUBLISH.ru.md) · [Call from Limbo](STYX-EXEC.md) · [Call from FlashVM](../../flashVM/doc/STYX_VFS_EXEC.md)

The bidirectional DisVM launcher can publish a Limbo handler at a path in
its live namespace. Load and initialize `styxexec.dis`, then use
[`publish`](styx-exec-publish.md) to claim an absent path under an existing
writable directory. The entry advertises execute permission. Use
[`dispatch`](styx-exec-dispatch.md) on the Limbo owner to run a callback
from a module implementing [`StyxexecHandler.invoke`](styx-exec-invoke.md).
One module instance can handle several paths.

The caller uses the original libstyxrpc execute protocol on the attached
session. Only the native dispatcher processes wire fragments. The owner
receives a copy of the request bytes in its own Dis thread. Each request
has an exact local reply ticket. Use [`recv`](styx-exec-recv.md) and
[`reply`](styx-exec-reply.md) or [`refuse`](styx-exec-refuse.md) when the
application dispatches work itself. The owner has up to five seconds to
answer a complete request. The wire client may have a shorter deadline.
The native dispatcher never enters the Limbo heap.

The local `/.execctl` file is reserved for publication. The mounted peer
opens the hidden `/.exec` service at the attachment root. Directory
iteration exposes the executable entry at its application path. Each
invocation checks which layer the live path selects. A shadowing bind
makes the original handler unreachable until its layer is visible again.
[`close`](styx-exec-close.md) withdraws the owner's exact registration and
preserves replacements at that path. An application should serialize
operations on one Node and run its handler in a dedicated thread when
it needs to handle other work at the same time.

## Run the paired check

1. Take the [paired build command](STYX-CHANNEL.md#acceptance) and add
   `--publish`. Use `--jit` and `--tcp` to select those modes.
2. [The Limbo application](../../lib/styxrpc/tests/flashvm/publish_pair.b)
   publishes two entries and starts the
   [AS movie](../../lib/styxrpc/tests/flashvm/publish_pair.as). The latter
   calls through the current directory and the absolute path.
3. [The compiled handler](../../lib/styxrpc/tests/flashvm/publish_handler.b)
   transforms short and 16 KiB requests. The movie calls the second
   entry after inspecting both replies. The Limbo application closes the
   entries and prints `PASS native publish RPC paired VMs`.

The runner builds the application and callback from Limbo and the movie
from AS. The paired native launcher is available on the hosted Linux
profile used by the test.
