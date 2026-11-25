# Mixed channel message schemas

[Русский](channel_messages.ru.md) · [Identities](../include/styxrpc/channel_messages.h) · [Custom messages](custom_messages.md)

Publish a named exchange using [styxrpc_messages_publish](messages_publish.md).
Use the version-one schema names below in the port's registry. Give each
outgoing schema a distinct nonzero tag and register a handler for each
accepted incoming schema. A direction may use any valid tags. The UniBus
declaration maps them to the exact name and version. Each open has its
own stream and can interleave these schemas.

| Schema | Payload |
| --- | --- |
| `styxrpc.int32` | One managed Int32 child. Use `styxrpc_value_integer` and `styxrpc_value_get_integer` with `STYXRPC_VALUE_INT`. |
| `styxrpc.real64` | One managed binary64 child. Use `styxrpc_value_real` and `styxrpc_value_get_real`. |
| `styxrpc.bool` | Exactly one byte, zero or one. |
| `styxrpc.text` | Counted UTF-8/WTF-8 bytes. Embedded zero bytes belong to the string. |
| `styxrpc.null` | Empty payload. |
| `styxrpc.ref.int32` | An [Int32 reference descriptor](int_variable_refs.md) relative to the server that owns this exchange session. |

[Managed codecs](../include/styxrpc/managed.h) define the numerical encoding.
The application decides which incoming kinds to accept and what to do with
them. Keep receive contexts alive through close and use the dispatcher owner
thread. Forward an Int32 descriptor only within its issuer session. A server
returning a descriptor can issue it with `styxrpc_int_ref_grant`. A
client's [messages_flush](messages_flush.md) sends without receiving.

[Reusable mailbox](exchange.md)
