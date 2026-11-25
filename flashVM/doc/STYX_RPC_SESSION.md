# fvm_styxrpc_session_bind

[Русский](STYX_RPC_SESSION.ru.md) · [Header](../include/flashvm/styxrpc_session.h) · [Source](../src/styxrpc/session.c) · [Close](STYX_RPC_SESSION_CLOSE.md)

Link `flashvm_styxrpc` and include `flashvm/styxrpc_session.h`.

```c
FVM_Result fvm_styxrpc_session_bind(FVM_Context *context, FVM_StyxRpcSession *binding,
    struct StyxClient *client, const StyxRpcProgram *program, FVM_Value *object);
```

On the context owner thread, supply a zero-initialized binding in stable host
storage and an already mounted session. A null program enables file I/O
and named export calls. For named launch, supply a program descriptor with a nonzero PID
in the session's reverse tree. Its optional parent PID and descriptor
mask are copied. The host publishes `.var`, `.fd` and `.vfs` before
serving the connection. Those callbacks can use host-owned data on a
separate dispatcher thread. FlashVM objects stay on their own owner thread.

On success the binding owns its launch tickets and `object` receives a new AS
object. Root or store that value before allocating other VM objects. The
binding is one-shot and borrows its client. Invalid input returns
[`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47). Other errors follow the FlashVM object allocation
and registration APIs. On failure the output is preserved.

| AS call | Result and contract |
| --- | --- |
| `launch(name, startup)` | Positive token on acknowledged delivery, −1 before sending, 0 if uncertain. `startup` is a counted string whose bytes, including NUL, are sent as-is. The named Styx service admits a complete frame of up to 64 bytes, including its context and framing. |
| `status(token)` | 1 pending, 2 successful completion, 3 failed completion, 0 uncertain. A confirmed terminal status is retained until close. |
| `close(token)` | True after retiring that exact token and sending clunk. A pending job receives a cancellation request. |
| `stat(path)` | `[status, lengthHigh, lengthLow, mode, directory]`. Length consists of two unsigned 32-bit words. Metadata fields are zero/false on failure. |
| `read(path, offset, count)` | `[status, bytes]`. Returns a dense Array containing the confirmed prefix, up to 2048 bytes. EOF is a successful short result. |
| `write(path, offset, bytes)` | `[status, confirmedCount]`. Accepts at most 2048 own indexed byte values in 0–255. Property reads can invoke getters before sending. A successful result can describe a short prefix. |
| [`callInt(module, method, argument)`](STYX_RPC_SESSION_CALL_INT.md) | Checked `fn(int): int` return value. |
| [`loadModule(path, ...methods)`](STYX_RPC_LOAD_MODULE.md) | Check and load named methods. |
| [`intVariable(path, writable)`](STYX_RPC_INT_VARIABLE.md) | Open a live Int32 proxy. |
| [`openMessages(root, ...types)`](STYX_RPC_OPEN_MESSAGES.md) | Open a named UniBus port. |

Up to eight tickets can be open at once. Tokens are exact positive AS
integers and are never reused during the binding's lifetime. Unknown or
closed tokens and malformed AS arguments produce an argument error.
The script cannot change the bound caller PID, parent or inherited FD policy.

File paths are relative to the mount, with no empty components, `.` or
`..`. AS offsets are exact integers from 0 to 2⁵³−1. For file operations,
status 0 means confirmed transfer, 1 means no operation on a file was
admitted, 2 means confirmed failure and 3 means uncertainty. Confirmed
bytes remain meaningful on partial failure. VM allocation failure after
a file operation does not authorize a replay. Named launch and file
operations are synchronous native calls using the session's I/O deadlines.
Use a dedicated owner thread that can wait.

Keep the binding alive while methods can be called. Close it before releasing
the session. See the [named launch test](../tests/styxrpc/launch.c) and
[VFS test](../tests/styxrpc/vfs.c).

For ordinary paths and long-lived file handles, use the [mounted filesystem](STYX_RPC_FILESYSTEM.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Context` | struct | [src/core/runtime.hpp:257](../src/core/runtime.hpp#L257) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_styxrpc_session_bind` | function | [include/flashvm/styxrpc_session.h:36](../include/flashvm/styxrpc_session.h#L36) |
| `FVM_StyxRpcSession` | struct | [include/flashvm/styxrpc_session.h:24](../include/flashvm/styxrpc_session.h#L24) |
| `FVM_Value` | struct | [include/flashvm/value.h:26](../include/flashvm/value.h#L26) |
<!-- source-links:end -->
