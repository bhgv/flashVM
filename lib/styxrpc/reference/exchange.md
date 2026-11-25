# Mixed message mailbox

[Русский](exchange.ru.md) · [UniBus](channel_messages.md)

Use the portable StyxRpc::rpc library for the mailbox and its schemas. Register the spec
at a named server root or on a client fid through the hosted SDK. The mailbox copies at
most 1024 complete records and twice the maximum payload bytes. Exhausting either bound
fails the delivering port. An owner thread must serialize port I/O, mailbox methods and
lifecycle hooks. Clients drive transport with [pump](messages_pump.md) or
[flush](messages_flush.md).

- [styxrpc_exchange_create](exchange_create.md)
- [styxrpc_exchange_destroy](exchange_destroy.md)
- [styxrpc_exchange_spec](exchange_spec.md)
- [styxrpc_exchange_attach](exchange_attach.md)
- [styxrpc_exchange_send](exchange_send.md)
- [styxrpc_exchange_pending](exchange_pending.md)
- [styxrpc_exchange_recv](exchange_recv.md)
