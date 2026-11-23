# styxrpc_program_bigtable

[Русский](program_bigtable.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/bigtable.h>
int styxrpc_program_bigtable(StyxRpcPrograms *programs, uint32_t pid, const char *name,
    const StyxRpcBigTable *table);
```

Publish `.prog/<pid>/.var/<name>` on the reverse Styx tree. Supply an initialized
`programs` with a registered `pid`, a unique valid child `name`, and a nonnull
`table` with its `apply` function. A read-only registration consumes six slots
and a writable registration consumes ten. The library copies the name and
borrows the descriptor and callback context through closure of the server and
its open fids. Use a separate program tree per attachment for independent selectors.

Open `.idx` for write and send one key at offset zero. A numeric key is one
zero byte followed by an eight-byte signed big-endian integer. A text key is
one byte of value 1, followed by 0 to 255 UTF-8 bytes. Embedded zero bytes are
allowed. For a relative numeric key, the callback resolves a negative index
against the dense prefix and returns a canonical nonnegative key on SELECT.
Keep that fid open while opening and using `.val`. Closing it revokes
dependent value fids. Write a complete eight-byte value at offset zero or
read it in slices.

| File | Meaning |
| --- | --- |
| `.type` | Text `table of big` and newline |
| `.len` | Decimal dense prefix length and newline |
| `.count` | Decimal total number of occupied positions and keys and newline |
| `.ins` | One write with signed position then signed value, eight bytes each |
| `.push` | One write with an eight-byte signed value |
| `.pop` | First nonempty read removes the last dense element and caches its eight-byte signed value |
| `.add` | One write with a complete typed key followed by an eight-byte signed value |

Set `writable` to publish the four mutation files and enable writes to `.val`.
The callback returns 1 for a complete operation and 0 on refusal without effect.
It owns storage, absent-key behavior and atomic mutations. SELECT does not
mutate the table and borrows text bytes only for the current operation.
Serialize the dispatcher with other native users of that storage.

The registration returns 1 on success. It returns 0 on invalid argument, name
collision, unknown process or insufficient slots. No nodes are published on
failure. Link `StyxRpc::serve`.

[styxrpc_program_collection](program_collection.md) · [Example](../examples/disvm/hybrid.c) · [Source](../src/program.c)
