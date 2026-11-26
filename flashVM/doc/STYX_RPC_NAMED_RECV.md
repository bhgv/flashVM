# Named channel recv()

[Русский](STYX_RPC_NAMED_RECV.ru.md) · [vfs.chan](STYX_RPC_NAMED_CHANNEL.md) · [Code](../src/styxrpc/session_named_channel.cpp)

`port.recv()` takes no arguments. It waits for a write from another open
participant on the same name and returns the decoded value. The next
pending writer in arrival order is selected. An issued reference on the
same attachment becomes a live alias. Assignment to that alias writes to
the issuing cell. A closed channel object, broken connection, or malformed
frame raises an error.
