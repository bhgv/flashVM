# Named channel write(value)

[Русский](STYX_RPC_NAMED_WRITE.ru.md) · [rpc.chan](STYX_RPC_NAMED_CHANNEL.md) · [Code](../src/styxrpc/session_named_values.cpp)

`port.write(value)` waits until a reader on another participant accepts
the value and returns `undefined`. An ordinary variable supplies its current
value. Strings, numbers, Booleans, null and undefined use portable tags. Arrays,
tables and objects use the FlashVM graph codec, including supported shared
edges and cycles. One encoded frame may contain up to 65,536 bytes including
its type byte. Values without a graph encoding and oversized frames raise an
error before sending.

`port.write(ref counter)` forwards an already issued same-attachment Int32
reference as a descriptor. Its receiver keeps a live proxy. Publishing a fresh
local AS cell as a remotely shared cell and references to other cell types
require additional issuer support.
