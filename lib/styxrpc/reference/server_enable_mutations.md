# styxrpc_server_enable_mutations

[Русский](server_enable_mutations.ru.md) · [SDK guide](../README.md) · [Create a server](server_create.md)

## Syntax

```c
#include <styxrpc/serve.h>

int styxrpc_server_enable_mutations(StyxRpcServer *server,
    const StyxRpcMutations *mutations);
```

Available in the hosted C99 build, target `StyxRpc::serve`.

## Parameters and result

| Parameter | Contract |
| --- | --- |
| `server` | An ordinary callback-tree server from [styxrpc_server_create](server_create.md). Call on its owner before the first dispatch step. |
| `mutations` | Immutable callback table borrowed through server destruction. Supply either callback or both. A missing callback leaves that operation disabled. |

Returns `1` after installation. Invalid arguments, an earlier installation, dispatch already started, and service-only servers return `0`. The function performs no transport I/O.

## Wire behavior and ownership

| 9P request | Callback and result |
| --- | --- |
| `Tcreate`, file `0666`, `OWRITE` | `create(context, parent, name, 0, id, revision, cookie)` publishes a file and transfers an already-open writer cookie. The directory fid becomes this file's fid and starts at position zero. The existing VFS write and close callbacks own that cookie. |
| `Tcreate`, directory `DMDIR\|0777`, `OREAD` | `create(context, parent, name, 1, id, revision, NULL)` publishes an empty directory. The same fid opens its directory cursor at position zero. |
| `Tremove` | `remove(context, id)` unlinks the exact live node. The dispatcher consumes the request's fid on success and on error. Other open cookies keep their original lifetimes. |

All callbacks run synchronously on the server owner, with the original tree's borrowed `context`. `parent` and `id` name stable nodes, and `name` is a single borrowed component. The adapter requires the parent directory's other-class write and search bits. Root removal, reserved service nodes and duplicate creation are refused. Callback implementations must also reject duplicates atomically, reject removal of nonempty directories and return `0` without changes on refusal.

On a successful create, return `1` and supply a fresh nonzero ordinary ID, the initial Qid revision and, for a file, the cookie. Make the child discoverable immediately through stat and enumeration and advance the parent's directory revision on either mutation. Keep IDs unique for the server lifetime, including after a name is recreated. When an unlink succeeds, retain any data still owned by other open cookies through their individual closes.

Callback duration is controlled by the provider. Avoid reentering the server from its callbacks. A disconnected client or a flushed request can lose its acknowledgement after an effect. Check current state before taking further action.

## Run the example

Configure with `-DSTYXRPC_EXAMPLES=ON -DBUILD_TESTING=ON` and a built Styx package. The provider keeps separate IDs and byte storage for removed nodes. The C caller creates a directory, writes and unlinks a child, and reads through the already-open fid. With the DisVM integration configured, the Limbo caller exercises the same provider using `Sys.create` and `Sys.remove`.

```sh
ctest --test-dir build-styxrpc -R 'styxrpc-(mutable-tree|disvm-mutations)' --output-on-failure
```

The selected tests print `PASS` and wait for the provider to check that all open cookies retired. See the [example walkthrough](../examples/disvm/MUTATIONS.md), [header](../include/styxrpc/serve.h) and [implementation](../src/serve_mutations.inc).
