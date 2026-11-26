# rpc.newchan

[Русский](STYX_RPC_NEWCHAN.ru.md) · [Default session](STYX_RPC_DEFAULT.md) · [Implementation](../src/styxrpc/session_channel.c)

`rpc.newchan(address)` is reserved for an additional session with a
different peer. The address is a string, for example Inferno's
`tcp!host!port`. The final address grammar is pending. The current method
accepts a nonempty string without NUL and returns
`undefined`. It starts no transport operation.
