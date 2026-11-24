# styxrpc_messages_pump

[Русский](messages_pump.ru.md) · [Header](../include/styxrpc/messages.h) · [Implementation](../src/message_client.c)

Include `styxrpc/messages.h` and link `StyxRpc::launch`.

```c
StyxRpcStatus styxrpc_messages_pump(StyxRpcMessagePort *port);
```

Pass a live client `port` on its owner thread outside callbacks using the
same session. The pump writes queued bytes and reads up to 4,096 bytes per
call, split into regular Styx transfers with their own deadlines. It
retains a partial record for the next call. Receive callbacks run inline
after complete validation. Output they queue is available on the next pump.

`STYXRPC_OK` means at least one user message was delivered. `STYXRPC_MORE`
means none was ready, even if the call moved bytes. `STYXRPC_INVALID` on
I/O, framing or receiver error retires the port. Earlier callbacks may
already have run. The caller still owns the port and must close it.
Passing a server port, NULL or calling recursively on the port also
returns `STYXRPC_INVALID`. The SDK does not replay writes with uncertain
outcome.

[Custom message setup](custom_messages.md) · [Worked socket example](../tests/message_socket.c)
