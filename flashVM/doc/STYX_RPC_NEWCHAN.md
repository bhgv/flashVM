# rpc.newchan

[Русский](STYX_RPC_NEWCHAN.ru.md) · [Session](STYX_RPC_SESSION.md) · [Implementation](../src/styxrpc/session_channel.c) · [Host the application exchange](STYX_RPC_LISTEN.md)

The paired FlashVM player exposes its attached Styx session as `rpc`. Open
one UniBus exchange to pass values and live references through the same fid.
An embedded player can expose the session object returned by
[`fvm_styxrpc_session_bind`](STYX_RPC_SESSION.md) under the same name.

`rpc.newchan(address)` takes a nonempty peer exchange root name, such as
`"app-events"` or `"/app-events"`. The name is resolved at the root of the
attached session, independent of the current directory. A peer publishes
that name through [styxrpc_messages_publish](../../lib/styxrpc/reference/messages_publish.md)
with the [mixed channel schemas](../../lib/styxrpc/reference/channel_messages.md).
Use one path component up to 255 bytes with an optional leading slash.
The underlying Styx session can use either a local or remote transport.

The result owns one dedicated fid. Its methods are [send](STYX_RPC_CHAN_SEND.md),
[recv](STYX_RPC_CHAN_RECV.md) and [close](STYX_RPC_CHAN_CLOSE.md). Opening
uses the session's Styx request deadline and fails if the address is
invalid or a published exchange cannot be opened. The peer's wire
declaration is processed during receive.

The following AS2 code expects the provider to issue its own Int32
reference as the first message, then handle both a value and a
reference for the original cell.

```actionscript
function onEnterFrame() {
    if (!sent) {
        var ch = rpc.newchan("/app-events");
        var counter = ch.recv();
        var sent = ch.send(counter);
        var forwarded = ch.send(ref counter);
        var closed = ch.close();
    }
}
```

Use the [test provider](../tests/styxrpc/variable-ref.c) for a working
echo contract. Poll again on a later frame if the first receive has no
complete message. For application-defined schema names and callbacks,
use [openMessages](STYX_RPC_OPEN_MESSAGES.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_styxrpc_session_bind` | function | [include/flashvm/styxrpc_session.h:36](../include/flashvm/styxrpc_session.h#L36) |
<!-- source-links:end -->


In the AS2 View profile, use a frame handler for reference operations.
Reads of initializer FIR temporaries currently dereference their contents.
For an end-to-end check, build [peer-newchan.as](../tests/styxrpc/peer-newchan.as)
with [AS2 View, FIR and SWL](../../compiler/as2/view/README.md) and pass the
SWF path as the first argument to `flashvm_tests_styxrpc_variable_ref`.
The test checks that the snapshot stays at 77 while assigning through the
forwarded reference changes the issuing cell to 111.
