# fvm_styxrpc_session_close

[Русский](STYX_RPC_SESSION_CLOSE.ru.md) · [Header](../include/flashvm/styxrpc_session.h) · [Source](../src/styxrpc/session.c) · [Binding](STYX_RPC_SESSION.md)

`void fvm_styxrpc_session_close(FVM_StyxRpcSession *binding);`

Call on the FlashVM context owner thread while no session method is active.
The function disarms the AS object, closes every owned launch fid and
retires every token. Clunk requests cancellation for any still-running
launch. The host keeps ownership of the mounted client and the reverse
registry and may close them after return. Keep the binding storage valid
through the last callable reference to its AS object. NULL and repeated
close on the same binding are harmless. The binding cannot be registered
again. The function may wait for the session's clunk operations.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_styxrpc_session_close` | function | [include/flashvm/styxrpc_session.h:42](../include/flashvm/styxrpc_session.h#L42) |
| `FVM_StyxRpcSession` | struct | [include/flashvm/styxrpc_session.h:24](../include/flashvm/styxrpc_session.h#L24) |
<!-- source-links:end -->
