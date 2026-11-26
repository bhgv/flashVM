# Publish VFS file callbacks from Limbo

[Русский](STYX-NODES.ru.md) · [Executable-only API](STYX-EXEC-PUBLISH.md) · [ActionScript file API](../../flashVM/doc/STYX_RPC_OS.md)

A Limbo application in the paired DisVM can make a callback available
as an ordinary entry in its live namespace. The file is listed
alongside other entries. Its name and permissions are visible through
stat. Ordinary Styx reads and writes reach the Limbo owner. Execute
requests use the existing libstyxrpc execute service on the same session.

## Interface

| Function | Use |
| --- | --- |
| [`init`](styx-nodes-init.md) | Load the system module before publication. |
| [`publish`](styx-nodes-publish.md) | Claim a free absolute path with any nonempty subset of `rwx`. |
| [`Node.dispatch`](styx-nodes-dispatch.md) | Handle one request through a loaded Limbo module. |
| [`StyxnodeHandler.transfer`](styx-nodes-transfer.md) | Fill a buffer or acknowledge the processed input prefix. |
| [`Node.recv`](styx-nodes-recv.md), [`Node.reply`](styx-nodes-reply.md), [`Node.refuse`](styx-nodes-refuse.md) | Receive or finish a request explicitly. |
| [`Node.close`](styx-nodes-close.md) | Release the registration. |

Use [positioned publication](STYX-NODE-OFFSETS.md) for a fixed length
and offsets. The original `publish` creates a sequential stream.

Read and write opens are sequential. The callback receives one
transport fragment at a time and may return fewer bytes than offered.
A zero count on read is EOF. The next read may ask for new data.
A positive short count allows the library to request the remaining
bytes. Each fragment is at most 65,536 bytes, with the negotiated Styx
message size imposing a smaller limit on ordinary remote I/O. The
file reports length zero. The callback has no byte offset, so an
operation at a different position fails. Opening a new file handle
starts a fresh transport cursor. Application state belongs to the
Limbo handler and can be shared across opens.

The native owner wait for each fragment is bounded by five seconds,
including time waiting for a previous fragment. The remote caller can
time out sooner. A caller may see an error after the owner has already
changed application state. The owner must answer before synchronously
calling back into the same waiting peer. The local control fid and
peer execute service are reserved. The application entry is resolved
through the normal VFS. A previously open file keeps its original
provider across namespace changes and fails after that owner retires.

## Run the paired check

1. Use the [paired build command](STYX-CHANNEL.md#acceptance) with
   `--nodes`. Add `--tcp` for TCP or `--jit` for the JITs.
2. The [Limbo entry module](../../lib/styxrpc/tests/flashvm/nodes_pair.b)
   loads `styxnodes.dis`, initializes it, publishes `/tools/mail` with
   `"rwx"`, and starts the compiled SWF through the paired player's
   upload and state files. It calls `node.dispatch(handler)` once for
   every transport request.
3. The [AS movie](../../lib/styxrpc/tests/flashvm/nodes_pair.as) finds the
   file with `os.opendir`, opens it with `vfs.open`, sends four bytes,
   reads two binary bytes, then calls `os.exec`.
   The [handler](../../lib/styxrpc/tests/flashvm/nodes_handler.b) confirms
   two write bytes at a time, fills the read buffer, and returns a short
   execute reply. The movie verifies each result, then signals completion.
4. The Limbo program tests refusal and withdrawal against an open file,
   closes every registration and prints `PASS native nodes RPC paired VMs`.

The driver uses the project's Limbo and AS compilers, FIR tools and SWL
linker. Both scripts run in the stock hosted Linux pair.
